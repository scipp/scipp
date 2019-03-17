import unittest

from dataset import *
import numpy as np
import hashlib
import io
from contextlib import redirect_stdout
import re

def plotly_output_to_md5sum(instring):
    stripped = re.sub(r"[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}", "", instring)
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
        self.assertEqual(plotly_output_to_md5sum(output), "5ae8b7cb6c7d2e0ace82e84b0bb31dd5")
        d1[Data.Variance, "Counts"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        with io.StringIO() as buf, redirect_stdout(buf):
            plot(d1)
            output = buf.getvalue()
        self.assertEqual(plotly_output_to_md5sum(output), "829f7d729d9c50454fde2186209c6bfc")

    def test_plot_1d_bin_edges(self):
        d1 = Dataset()
        N = 100
        d1[Coord.Tof] = ([Dim.Tof], np.arange(N+1).astype(np.float64))
        d1[Data.Value, "Counts"] = ([Dim.Tof], np.arange(N).astype(np.float64)**2)
        with io.StringIO() as buf, redirect_stdout(buf):
            plot(d1)
            output = buf.getvalue()
        self.assertEqual(plotly_output_to_md5sum(output), "8c7e59534753294865e4816238fab2f6")

    def test_plot_1d_two_values(self):
        d1 = Dataset()
        N = 100
        d1[Coord.Tof] = ([Dim.Tof], np.arange(N).astype(np.float64))
        d1[Data.Value, "Counts"] = ([Dim.Tof], np.arange(N).astype(np.float64)**2)
        d1[Data.Variance, "Counts"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        d1[Data.Value, "Sample"] = ([Dim.Tof], np.sin(np.arange(N).astype(np.float64)))
        d1[Data.Variance, "Sample"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        with io.StringIO() as buf, redirect_stdout(buf):
            plot(d1)
            output = buf.getvalue()
        self.assertEqual(plotly_output_to_md5sum(output), "69ce8aaeb6c2f1c6c391a97c3155c80f")

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
        with io.StringIO() as buf, redirect_stdout(buf):
            plot([d1, d2])
            output = buf.getvalue()
        self.assertEqual(plotly_output_to_md5sum(output), "e3e357f8072c91fa33dbf8d5ffd06a8d")

    def test_plot_2d_image(self):
        d3 = Dataset()
        I = 100
        J = 200
        d3[Coord.Tof] = ([Dim.Tof], np.arange(I).astype(np.float64))
        d3[Coord.Position] = ([Dim.Position], np.arange(J).astype(np.float64))
        d3[Data.Value, "sample"] = ([Dim.Position, Dim.Tof], np.arange(I*J).reshape(J,I).astype(np.float64))
        d3[Data.Value, "sample"].unit = units.counts
        with io.StringIO() as buf, redirect_stdout(buf):
            plot(d3)
            output = buf.getvalue()
        self.assertEqual(plotly_output_to_md5sum(output), "e65afd85717d30a62533f11a7ef86cc7")
        d3[Coord.SpectrumNumber] = ([Dim.Position], np.arange(J).astype(np.float64))
        with io.StringIO() as buf, redirect_stdout(buf):
            plot(d3, axes=[Coord.SpectrumNumber, Coord.Tof])
            output = buf.getvalue()
        self.assertEqual(plotly_output_to_md5sum(output), "87dac3ee069df8f471f6c3452d0b9595")

    def test_plot_waterfall(self):
        d4 = Dataset()
        I = 100
        J = 30
        d4[Coord.Tof] = ([Dim.Tof], np.arange(I).astype(np.float64))
        d4[Coord.Position] = ([Dim.Position], np.arange(J).astype(np.float64))
        d4[Data.Value, "counts"] = ([Dim.Position, Dim.Tof], np.arange(I*J).reshape(J,I).astype(np.float64))
        with io.StringIO() as buf, redirect_stdout(buf):
            plot(d4, waterfall=Dim.Position)
            output = buf.getvalue()
        self.assertEqual(plotly_output_to_md5sum(output), "5ae5f9d69f22cd188a6a7eb969810dda")

    def test_plot_sliceviewer(self):
        d5 = Dataset()
        I = 10
        J = 20
        K = 30
        d5[Coord.X] = ([Dim.X], np.arange(I).astype(np.float64))
        d5[Coord.Y] = ([Dim.Y], np.arange(J).astype(np.float64))
        d5[Coord.Z] = ([Dim.Z], np.arange(K).astype(np.float64))
        d5[Data.Value, "background"] = ([Dim.Z, Dim.Y, Dim.X], np.arange(I*J*K).reshape(K,J,I).astype(np.float64))
        with io.StringIO() as buf, redirect_stdout(buf):
            print(plot(d5))
            output = buf.getvalue()
        self.assertEqual(plotly_output_to_md5sum(output), "4e9996c5d92f5702b4b3ea123b45a53c")

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
        with io.StringIO() as buf, redirect_stdout(buf):
            print(plot(d5))
            output = buf.getvalue()
        self.assertEqual(plotly_output_to_md5sum(output), "b08b903f36847141b72da5d519153c61")

    def test_plot_sliceviewer_with_axes(self):
        d5 = Dataset()
        I = 10
        J = 20
        K = 30
        d5[Coord.X] = ([Dim.X], np.arange(I).astype(np.float64))
        d5[Coord.Y] = ([Dim.Y], np.arange(J).astype(np.float64))
        d5[Coord.Z] = ([Dim.Z], np.arange(K).astype(np.float64))
        d5[Data.Value, "background"] = ([Dim.Z, Dim.Y, Dim.X], np.arange(I*J*K).reshape(K,J,I).astype(np.float64))
        with io.StringIO() as buf, redirect_stdout(buf):
            print(plot(d5, axes=[Coord.X, Coord.Z, Coord.Y]))
            output = buf.getvalue()
        self.assertEqual(plotly_output_to_md5sum(output), "0af166090ea34d41b58a1f146bf3552d")


if __name__ == '__main__':
    unittest.main()
