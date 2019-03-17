import unittest

from dataset import *
import numpy as np
import hashlib
import io
from contextlib import redirect_stdout
import re

def plot_to_md5sum(d, axes=None, waterfall=None):
    with io.StringIO() as buf, redirect_stdout(buf):
        print(plot(d, axes=axes, waterfall=waterfall))
        output = buf.getvalue()
    stripped = re.sub(r"[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}", "", output)
    return hashlib.md5(stripped.encode('utf-8')).hexdigest()


class TestPlotting(unittest.TestCase):

    def test_plot_1d(self):
        d1 = Dataset()
        N = 100
        d1[Coord.Tof] = ([Dim.Tof], np.arange(N).astype(np.float64))
        d1[Data.Value, "Counts"] = ([Dim.Tof], np.arange(N).astype(np.float64)**2)
        with io.StringIO() as buf, redirect_stdout(buf):
            plot(d1)
            output = buf.getvalue()
        self.assertEqual(plot_to_md5sum(d1), "7ef66658d86ced5d5bc7b4825b7fbe41")
        d1[Data.Variance, "Counts"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        self.assertEqual(plot_to_md5sum(d1), "07f58af06cd849ad34fd0daaa5baa6e8")

    def test_plot_1d_bin_edges(self):
        d1 = Dataset()
        N = 100
        d1[Coord.Tof] = ([Dim.Tof], np.arange(N+1).astype(np.float64))
        d1[Data.Value, "Counts"] = ([Dim.Tof], np.arange(N).astype(np.float64)**2)
        self.assertEqual(plot_to_md5sum(d1), "1da9042ff64c00f2a08f32048df2738e")

    def test_plot_1d_two_values(self):
        d1 = Dataset()
        N = 100
        d1[Coord.Tof] = ([Dim.Tof], np.arange(N).astype(np.float64))
        d1[Data.Value, "Counts"] = ([Dim.Tof], np.arange(N).astype(np.float64)**2)
        d1[Data.Variance, "Counts"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        d1[Data.Value, "Sample"] = ([Dim.Tof], np.sin(np.arange(N).astype(np.float64)))
        d1[Data.Variance, "Sample"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        self.assertEqual(plot_to_md5sum(d1), "430c3877e0061e1cb24bf20afa5408be")

    def test_plot_1d_list_of_datasets(self):
        d1 = Dataset()
        N = 100
        d1[Coord.Tof] = ([Dim.Tof], np.arange(N).astype(np.float64))
        d1[Data.Value, "Counts"] = ([Dim.Tof], np.arange(N).astype(np.float64)**2)
        d1[Data.Variance, "Counts"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        d2 = Dataset()
        d2[Coord.Tof] = ([Dim.Tof], np.arange(N).astype(np.float64))
        d2[Data.Value, "Sample"] = ([Dim.Tof], np.cos(np.arange(N).astype(np.float64)))
        d2[Data.Variance, "Sample"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        self.assertEqual(plot_to_md5sum([d1, d2]), "d23c46c0bca3b32280dde845f2d7234e")

    def test_plot_2d_image(self):
        d3 = Dataset()
        I = 100
        J = 200
        d3[Coord.Tof] = ([Dim.Tof], np.arange(I).astype(np.float64))
        d3[Coord.Position] = ([Dim.Position], np.arange(J).astype(np.float64))
        d3[Data.Value, "sample"] = ([Dim.Position, Dim.Tof], np.arange(I*J).reshape(J,I).astype(np.float64))
        d3[Data.Value, "sample"].unit = units.counts
        self.assertEqual(plot_to_md5sum(d3), "f63912573bcc67e396d9463f5586f43a")
        d3[Coord.SpectrumNumber] = ([Dim.Position], np.arange(J).astype(np.float64))
        self.assertEqual(plot_to_md5sum(d3, axes=[Coord.SpectrumNumber, Coord.Tof]), "46f0146dd9a4b8f091245e99bdcd55c6")

    def test_plot_waterfall(self):
        d4 = Dataset()
        I = 100
        J = 30
        d4[Coord.Tof] = ([Dim.Tof], np.arange(I).astype(np.float64))
        d4[Coord.Position] = ([Dim.Position], np.arange(J).astype(np.float64))
        d4[Data.Value, "counts"] = ([Dim.Position, Dim.Tof], np.arange(I*J).reshape(J,I).astype(np.float64))
        self.assertEqual(plot_to_md5sum(d4, waterfall=Dim.Position), "845871ba71f077e10db535751560fd11")

    def test_plot_sliceviewer(self):
        d5 = Dataset()
        I = 10
        J = 20
        K = 30
        d5[Coord.X] = ([Dim.X], np.arange(I).astype(np.float64))
        d5[Coord.Y] = ([Dim.Y], np.arange(J).astype(np.float64))
        d5[Coord.Z] = ([Dim.Z], np.arange(K).astype(np.float64))
        d5[Data.Value, "background"] = ([Dim.Z, Dim.Y, Dim.X], np.arange(I*J*K).reshape(K,J,I).astype(np.float64))
        self.assertEqual(plot_to_md5sum(d5), "4e9996c5d92f5702b4b3ea123b45a53c")

    def test_plot_sliceviewer_with_two_sliders(self):
        d5 = Dataset()
        I = 10
        J = 11
        K = 12
        L = 13
        d5[Coord.X] = ([Dim.X], np.arange(I).astype(np.float64))
        d5[Coord.Y] = ([Dim.Y], np.arange(J).astype(np.float64))
        d5[Coord.Z] = ([Dim.Z], np.arange(K).astype(np.float64))
        d5[Coord.Energy] = ([Dim.Energy], np.arange(L).astype(np.float64))
        d5[Data.Value, "sample"] = ([Dim.Energy, Dim.Z, Dim.Y, Dim.X], np.arange(I*J*K*L).reshape(L,K,J,I).astype(np.float64))
        self.assertEqual(plot_to_md5sum(d5), "b08b903f36847141b72da5d519153c61")

    def test_plot_sliceviewer_with_axes(self):
        d5 = Dataset()
        I = 10
        J = 20
        K = 30
        d5[Coord.X] = ([Dim.X], np.arange(I).astype(np.float64))
        d5[Coord.Y] = ([Dim.Y], np.arange(J).astype(np.float64))
        d5[Coord.Z] = ([Dim.Z], np.arange(K).astype(np.float64))
        d5[Data.Value, "background"] = ([Dim.Z, Dim.Y, Dim.X], np.arange(I*J*K).reshape(K,J,I).astype(np.float64))
        self.assertEqual(plot_to_md5sum(d5, axes=[Coord.X, Coord.Z, Coord.Y]), "0af166090ea34d41b58a1f146bf3552d")


if __name__ == '__main__':
    unittest.main()
