import unittest

from scippy import *
import numpy as np
import hashlib
import io
from contextlib import redirect_stdout
import re

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.

def do_plot(d, axes=None, waterfall=None):
    with io.StringIO() as buf, redirect_stdout(buf):
        plot(d, axes=axes, waterfall=waterfall)
    #     output = buf.getvalue()
    # # Plotly generates some unique ids for plot html elements every time it
    # # runs, so we need to strip them out for result reproducibility
    # stripped = re.sub(r"[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}", "", output)
    # return hashlib.md5(stripped.encode('utf-8')).hexdigest()
    return

class TestPlotting(unittest.TestCase):

    def test_plot_1d(self):
        d1 = Dataset()
        N = 100
        d1[Coord.Tof] = ([Dim.Tof], np.arange(N).astype(np.float64))
        d1[Data.Value, "Counts"] = ([Dim.Tof], np.arange(N).astype(np.float64)**2)
        do_plot(d1)
        # self.assertEqual(plot_to_md5sum(d1), "7ef66658d86ced5d5bc7b4825b7fbe41")
        d1[Data.Variance, "Counts"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        do_plot(d1)

    def test_plot_1d_bin_edges(self):
        d1 = Dataset()
        N = 100
        d1[Coord.Tof] = ([Dim.Tof], np.arange(N+1).astype(np.float64))
        d1[Data.Value, "Counts"] = ([Dim.Tof], np.arange(N).astype(np.float64)**2)
        do_plot(d1)

    def test_plot_1d_two_values(self):
        d1 = Dataset()
        N = 100
        d1[Coord.Tof] = ([Dim.Tof], np.arange(N).astype(np.float64))
        d1[Data.Value, "Counts"] = ([Dim.Tof], np.arange(N).astype(np.float64)**2)
        d1[Data.Variance, "Counts"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        d1[Data.Value, "Sample"] = ([Dim.Tof], np.sin(np.arange(N).astype(np.float64)))
        d1[Data.Variance, "Sample"] = ([Dim.Tof], 0.1*np.arange(N).astype(np.float64))
        do_plot(d1)

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
        do_plot([d1, d2])

    def test_plot_2d_image(self):
        d3 = Dataset()
        I = 100
        J = 200
        d3[Coord.Tof] = ([Dim.Tof], np.arange(I).astype(np.float64))
        d3[Coord.Position] = ([Dim.Position], np.arange(J).astype(np.float64))
        d3[Data.Value, "sample"] = ([Dim.Position, Dim.Tof], np.arange(I*J).reshape(J,I).astype(np.float64))
        d3[Data.Value, "sample"].unit = units.counts
        do_plot(d3)
        d3[Coord.SpectrumNumber] = ([Dim.Position], np.arange(J).astype(np.float64))
        do_plot(d3, axes=[Coord.SpectrumNumber, Coord.Tof])

    def test_plot_waterfall(self):
        d4 = Dataset()
        I = 100
        J = 30
        d4[Coord.Tof] = ([Dim.Tof], np.arange(I).astype(np.float64))
        d4[Coord.Position] = ([Dim.Position], np.arange(J).astype(np.float64))
        d4[Data.Value, "counts"] = ([Dim.Position, Dim.Tof], np.arange(I*J).reshape(J,I).astype(np.float64))
        do_plot(d4, waterfall=Dim.Position)

    def test_plot_sliceviewer(self):
        d5 = Dataset()
        I = 10
        J = 20
        K = 30
        d5[Coord.X] = ([Dim.X], np.arange(I).astype(np.float64))
        d5[Coord.Y] = ([Dim.Y], np.arange(J).astype(np.float64))
        d5[Coord.Z] = ([Dim.Z], np.arange(K).astype(np.float64))
        d5[Data.Value, "background"] = ([Dim.Z, Dim.Y, Dim.X], np.arange(I*J*K).reshape(K,J,I).astype(np.float64))
        do_plot(d5)

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
        do_plot(d5)

    def test_plot_sliceviewer_with_axes(self):
        d5 = Dataset()
        I = 10
        J = 20
        K = 30
        d5[Coord.X] = ([Dim.X], np.arange(I).astype(np.float64))
        d5[Coord.Y] = ([Dim.Y], np.arange(J).astype(np.float64))
        d5[Coord.Z] = ([Dim.Z], np.arange(K).astype(np.float64))
        d5[Data.Value, "background"] = ([Dim.Z, Dim.Y, Dim.X], np.arange(I*J*K).reshape(K,J,I).astype(np.float64))
        do_plot(d5, axes=[Coord.X, Coord.Z, Coord.Y])

if __name__ == '__main__':
    unittest.main()
