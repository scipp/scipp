# @file
# SPDX-License-Identifier: GPL-3.0-or-later
# @author Neil Vaytet
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
import unittest

import scippy as sp
import numpy as np
import io
from contextlib import redirect_stdout

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.


def do_plot(d, axes=None, waterfall=None):
    with io.StringIO() as buf, redirect_stdout(buf):
        sp.plot(d, axes=axes, waterfall=waterfall)
    return


class TestPlotting(unittest.TestCase):

    def test_plot_1d(self):
        d1 = sp.Dataset()
        N = 100
        d1[sp.Coord.Tof] = ([sp.Dim.Tof], np.arange(N).astype(np.float64))
        d1[sp.Data.Value, "Counts"] = (
            [sp.Dim.Tof], np.arange(N).astype(np.float64)**2)
        do_plot(d1)
        d1[sp.Data.Variance, "Counts"] = (
            [sp.Dim.Tof], 0.1 * np.arange(N).astype(np.float64))
        do_plot(d1)

    def test_plot_1d_bin_edges(self):
        d1 = sp.Dataset()
        N = 100
        d1[sp.Coord.Tof] = ([sp.Dim.Tof], np.arange(N + 1).astype(np.float64))
        d1[sp.Data.Value, "Counts"] = (
            [sp.Dim.Tof], np.arange(N).astype(np.float64)**2)
        do_plot(d1)

    def test_plot_1d_two_values(self):
        d1 = sp.Dataset()
        N = 100
        d1[sp.Coord.Tof] = ([sp.Dim.Tof], np.arange(N).astype(np.float64))
        d1[sp.Data.Value, "Counts"] = (
            [sp.Dim.Tof], np.arange(N).astype(np.float64)**2)
        d1[sp.Data.Variance, "Counts"] = (
            [sp.Dim.Tof], 0.1 * np.arange(N).astype(np.float64))
        d1[sp.Data.Value, "Sample"] = (
            [sp.Dim.Tof], np.sin(np.arange(N).astype(np.float64)))
        d1[sp.Data.Variance, "Sample"] = (
            [sp.Dim.Tof], 0.1 * np.arange(N).astype(np.float64))
        do_plot(d1)

    def test_plot_1d_list_of_datasets(self):
        d1 = sp.Dataset()
        N = 100
        d1[sp.Coord.Tof] = ([sp.Dim.Tof], np.arange(N).astype(np.float64))
        d1[sp.Data.Value, "Counts"] = (
            [sp.Dim.Tof], np.arange(N).astype(np.float64)**2)
        d1[sp.Data.Variance, "Counts"] = (
            [sp.Dim.Tof], 0.1 * np.arange(N).astype(np.float64))
        d2 = sp.Dataset()
        d2[sp.Coord.Tof] = ([sp.Dim.Tof], np.arange(N).astype(np.float64))
        d2[sp.Data.Value, "Sample"] = (
            [sp.Dim.Tof], np.cos(np.arange(N).astype(np.float64)))
        d2[sp.Data.Variance, "Sample"] = (
            [sp.Dim.Tof], 0.1 * np.arange(N).astype(np.float64))
        do_plot([d1, d2])

    def test_plot_2d_image(self):
        d3 = sp.Dataset()
        n1 = 100
        n2 = 200
        d3[sp.Coord.Tof] = ([sp.Dim.Tof], np.arange(n1).astype(np.float64))
        d3[sp.Coord.Position] = (
            [sp.Dim.Position], np.arange(n2).astype(np.float64))
        d3[sp.Data.Value, "sample"] = \
            ([sp.Dim.Position, sp.Dim.Tof],
             np.arange(n1 * n2).reshape(n2, n1).astype(np.float64))
        d3[sp.Data.Value, "sample"].unit = sp.units.counts
        do_plot(d3)
        d3[sp.Coord.SpectrumNumber] = (
            [sp.Dim.Position], np.arange(n2).astype(np.float64))
        do_plot(d3, axes=[sp.Coord.SpectrumNumber, sp.Coord.Tof])

    def test_plot_waterfall(self):
        d4 = sp.Dataset()
        n1 = 100
        n2 = 30
        d4[sp.Coord.Tof] = ([sp.Dim.Tof], np.arange(n1).astype(np.float64))
        d4[sp.Coord.Position] = (
            [sp.Dim.Position], np.arange(n2).astype(np.float64))
        d4[sp.Data.Value, "counts"] = \
            ([sp.Dim.Position, sp.Dim.Tof], np.arange(
             n1 * n2).reshape(n2, n1).astype(np.float64))
        do_plot(d4, waterfall=sp.Dim.Position)

    def test_plot_sliceviewer(self):
        d5 = sp.Dataset()
        n1 = 10
        n2 = 20
        n3 = 30
        d5[sp.Coord.X] = ([sp.Dim.X], np.arange(n1).astype(np.float64))
        d5[sp.Coord.Y] = ([sp.Dim.Y], np.arange(n2).astype(np.float64))
        d5[sp.Coord.Z] = ([sp.Dim.Z], np.arange(n3).astype(np.float64))
        d5[sp.Data.Value, "background"] = \
            ([sp.Dim.Z, sp.Dim.Y, sp.Dim.X], np.arange(
             n1 * n2 * n3).reshape(n3, n2, n1).astype(np.float64))
        do_plot(d5)

    def test_plot_sliceviewer_with_two_sliders(self):
        d5 = sp.Dataset()
        n1 = 10
        n2 = 11
        n3 = 12
        n4 = 13
        d5[sp.Coord.X] = ([sp.Dim.X], np.arange(n1).astype(np.float64))
        d5[sp.Coord.Y] = ([sp.Dim.Y], np.arange(n2).astype(np.float64))
        d5[sp.Coord.Z] = ([sp.Dim.Z], np.arange(n3).astype(np.float64))
        d5[sp.Coord.Energy] = (
            [sp.Dim.Energy], np.arange(n4).astype(np.float64))
        d5[sp.Data.Value, "sample"] = \
            ([sp.Dim.Energy, sp.Dim.Z, sp.Dim.Y, sp.Dim.X], np.arange(
             n1 * n2 * n3 * n4).reshape(n4, n3, n2, n1).astype(np.float64))
        do_plot(d5)

    def test_plot_sliceviewer_with_axes(self):
        d5 = sp.Dataset()
        n1 = 10
        n2 = 20
        n3 = 30
        d5[sp.Coord.X] = ([sp.Dim.X], np.arange(n1).astype(np.float64))
        d5[sp.Coord.Y] = ([sp.Dim.Y], np.arange(n2).astype(np.float64))
        d5[sp.Coord.Z] = ([sp.Dim.Z], np.arange(n3).astype(np.float64))
        d5[sp.Data.Value, "background"] = \
            ([sp.Dim.Z, sp.Dim.Y, sp.Dim.X], np.arange(
             n1 * n2 * n3).reshape(n3, n2, n1).astype(np.float64))
        do_plot(d5, axes=[sp.Coord.X, sp.Coord.Z, sp.Coord.Y])


if __name__ == '__main__':
    unittest.main()
