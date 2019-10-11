# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import scipp as sc
import numpy as np
import io
from contextlib import redirect_stdout
import pytest

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.


def do_plot(d, **kwargs):
    with io.StringIO() as buf, redirect_stdout(buf):
        sc.plot(d, **kwargs)
    return


def make_2d_dataset(variances=False):
    N = 100
    M = 50
    xx = np.arange(N, dtype=np.float64)
    yy = np.arange(M, dtype=np.float64)
    x, y = np.meshgrid(xx, yy)
    b = N / 20.0
    c = M / 2.0
    r = np.sqrt(((x - c) / b)**2 + ((y - c) / b)**2)
    a = np.sin(r)
    d1 = sc.Dataset(
        coords={
            sc.Dim.X: sc.Variable([sc.Dim.X], values=xx, unit=sc.units.m),
            sc.Dim.Y: sc.Variable([sc.Dim.Y], values=yy, unit=sc.units.m)
        })
    params = {"values": a}
    if variances:
        params["variances"] = np.random.rand(M, N) + (x == y)
    d1["Sample"] = sc.Variable([sc.Dim.Y, sc.Dim.X],
                               unit=sc.units.counts,
                               **params)
    return d1


def do_test_plot_1d(**kwargs):
    d1 = sc.Dataset()
    N = 100
    d1.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sc.units.us)
    d1["Sample"] = sc.Variable([sc.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               unit=sc.units.counts)
    do_plot(d1, **kwargs)


def do_test_plot_1d_with_variances():
    d1 = sc.Dataset()
    N = 100
    d1.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sc.units.us)
    d1["Sample"] = sc.Variable([sc.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               variances=np.random.rand(N),
                               unit=sc.units.counts)
    do_plot(d1)


def do_test_plot_1d_bin_edges():
    d1 = sc.Dataset()
    N = 100
    d1.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        values=np.arange(N + 1).astype(
                                            np.float64),
                                        unit=sc.units.us)
    d1["Sample"] = sc.Variable([sc.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               unit=sc.units.counts)
    do_plot(d1)


def do_test_plot_1d_bin_edges_with_variances():
    d1 = sc.Dataset()
    N = 100
    d1.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        values=np.arange(N + 1).astype(
                                            np.float64),
                                        unit=sc.units.us)
    d1["Sample"] = sc.Variable([sc.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               variances=np.random.rand(N),
                               unit=sc.units.counts)
    do_plot(d1)


def do_test_plot_1d_two_entries():
    d1 = sc.Dataset()
    N = 100
    d1.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sc.units.us)
    d1["Sample"] = sc.Variable([sc.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               unit=sc.units.counts)
    d1["Background"] = sc.Variable([sc.Dim.Tof],
                                   values=2.0 * np.random.rand(N),
                                   unit=sc.units.counts)
    do_plot(d1)


def do_test_plot_1d_list_of_datasets():
    N = 100
    d1 = sc.Dataset()
    d1.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sc.units.us)
    d1["Sample"] = sc.Variable([sc.Dim.Tof], values=10.0 * np.random.rand(N))
    d1["Background"] = sc.Variable([sc.Dim.Tof],
                                   values=2.0 * np.random.rand(N))
    d2 = sc.Dataset()
    d2.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sc.units.us)
    d2["Sample"] = sc.Variable([sc.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               variances=np.random.rand(N))
    d2["Background"] = sc.Variable([sc.Dim.Tof],
                                   values=2.0 * np.random.rand(N),
                                   variances=np.random.rand(N))
    do_plot([d1, d2])


def do_test_plot_2d_image():
    d1 = make_2d_dataset()
    do_plot(d1)


def do_test_plot_2d_image_with_axes():
    d1 = make_2d_dataset()
    do_plot(d1, axes=[sc.Dim.X, sc.Dim.Y])


def do_test_plot_2d_image_with_variances():
    d1 = make_2d_dataset(variances=True)
    do_plot(d1)


def do_test_plot_2d_image_with_filename(fname):
    d1 = make_2d_dataset()
    do_plot(d1, filename=fname)


def do_test_plot_2d_image_with_variances_with_filename(fname):
    d1 = make_2d_dataset(variances=True)
    do_plot(d1, filename=fname)


def do_test_plot_collapse():
    N = 100
    M = 5
    d1 = sc.Dataset()
    d1.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        values=np.arange(N + 1).astype(
                                            np.float64),
                                        unit=sc.units.us)
    d1.coords[sc.Dim.X] = sc.Variable([sc.Dim.X],
                                      values=np.arange(M).astype(np.float64),
                                      unit=sc.units.m)
    d1["Sample"] = sc.Variable([sc.Dim.X, sc.Dim.Tof],
                               values=10.0 * np.random.rand(M, N),
                               variances=np.random.rand(M, N))
    do_plot(d1, collapse=sc.Dim.Tof)


def do_test_plot_waterfall():
    N = 100
    M = 5
    d1 = sc.Dataset()
    d1.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        values=np.arange(N + 1).astype(
                                            np.float64),
                                        unit=sc.units.us)
    d1.coords[sc.Dim.X] = sc.Variable([sc.Dim.X],
                                      values=np.arange(M).astype(np.float64),
                                      unit=sc.units.m)
    d1["Sample"] = sc.Variable([sc.Dim.X, sc.Dim.Tof],
                               values=10.0 * np.random.rand(M, N),
                               variances=np.random.rand(M, N))
    do_plot(d1, waterfall=sc.Dim.X)


def do_test_plot_sliceviewer():
    d1 = sc.Dataset()
    n1 = 20
    n2 = 30
    n3 = 40
    d1.coords[sc.Dim.X] = sc.Variable([sc.Dim.X],
                                      np.arange(n1).astype(np.float64))
    d1.coords[sc.Dim.Y] = sc.Variable([sc.Dim.Y],
                                      np.arange(n2).astype(np.float64))
    d1.coords[sc.Dim.Z] = sc.Variable([sc.Dim.Z],
                                      np.arange(n3).astype(np.float64))
    d1["Sample"] = sc.Variable([sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
                               values=np.arange(n1 * n2 * n3).reshape(
                                   n3, n2, n1).astype(np.float64))
    do_plot(d1)


def do_test_plot_sliceviewer_with_two_sliders():
    d1 = sc.Dataset()
    n1 = 20
    n2 = 30
    n3 = 40
    n4 = 50
    d1.coords[sc.Dim.X] = sc.Variable([sc.Dim.X],
                                      np.arange(n1).astype(np.float64))
    d1.coords[sc.Dim.Y] = sc.Variable([sc.Dim.Y],
                                      np.arange(n2).astype(np.float64))
    d1.coords[sc.Dim.Z] = sc.Variable([sc.Dim.Z],
                                      np.arange(n3).astype(np.float64))
    d1.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        np.arange(n4).astype(np.float64))
    d1["Sample"] = sc.Variable([sc.Dim.Tof, sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
                               values=np.arange(n1 * n2 * n3 * n4).reshape(
                                   n4, n3, n2, n1).astype(np.float64))
    do_plot(d1)


def do_test_plot_sliceviewer_with_axes():
    d1 = sc.Dataset()
    n1 = 20
    n2 = 30
    n3 = 40
    d1.coords[sc.Dim.X] = sc.Variable([sc.Dim.X],
                                      np.arange(n1).astype(np.float64))
    d1.coords[sc.Dim.Y] = sc.Variable([sc.Dim.Y],
                                      np.arange(n2).astype(np.float64))
    d1.coords[sc.Dim.Z] = sc.Variable([sc.Dim.Z],
                                      np.arange(n3).astype(np.float64))
    d1["Sample"] = sc.Variable([sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
                               values=np.arange(n1 * n2 * n3).reshape(
                                   n3, n2, n1).astype(np.float64))
    do_plot(d1, axes=[sc.Dim.Y, sc.Dim.X, sc.Dim.Z])


# Using plotly backend =======================================================

def test_plot_1d():
    do_test_plot_1d()


def test_plot_1d_with_variances():
    do_test_plot_1d_with_variances()


def test_plot_1d_bin_edges():
    do_test_plot_1d_bin_edges()


def test_plot_1d_bin_edges_with_variances():
    do_test_plot_1d_bin_edges_with_variances()


def test_plot_1d_two_entries():
    do_test_plot_1d_bin_edges_with_variances()


def test_plot_1d_list_of_datasets():
    do_test_plot_1d_list_of_datasets()


def test_plot_2d_image():
    do_test_plot_2d_image()


@pytest.mark.skip(reason="This test fails from time to time because objects "
                  "are running out of scope, so we disable it for now. "
                  "More specifically, in plot.py L306:\n"
                  "  if (zlabs[0] == xlabs[0]) and "
                  "(zlabs[1] == ylabs[0]):\n"
                  "xlabs and ylabs sometimes contains the same thing, "
                  "when they should in fact be different")
def test_plot_2d_image_with_axes():
    do_test_plot_2d_image_with_axes()


def test_plot_2d_image_with_variances():
    do_test_plot_2d_image_with_variances()


def test_plot_2d_image_with_filename():
    do_test_plot_2d_image_with_filename("test.html")


def test_plot_2d_image_with_variances_with_filename():
    do_test_plot_2d_image_with_variances_with_filename(["values.html",
                                                        "errors.html"])


def test_plot_collapse():
    do_test_plot_collapse()


def test_plot_waterfall():
    do_test_plot_waterfall()


def test_plot_sliceviewer():
    do_test_plot_sliceviewer()


def test_plot_sliceviewer_with_two_sliders():
    do_test_plot_sliceviewer_with_two_sliders()


def test_plot_sliceviewer_with_axes():
    do_test_plot_sliceviewer_with_axes()


# Using matplotlib backend ====================================================

def test_plot_1d_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_1d()


def test_plot_1d_with_variances_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_1d_with_variances()


def test_plot_1d_bin_edges_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_1d_bin_edges()


def test_plot_1d_bin_edges_with_variances_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_1d_bin_edges_with_variances()


def test_plot_1d_two_entries_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_1d_bin_edges_with_variances()


def test_plot_1d_list_of_datasets_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_1d_list_of_datasets()


def test_plot_2d_image_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_2d_image()


def test_plot_2d_image_with_axes_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_2d_image_with_axes()


def test_plot_2d_image_with_variances_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_2d_image_with_variances()


def test_plot_2d_image_with_filename_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_2d_image_with_filename("image.pdf")


def test_plot_2d_image_with_variances_with_filename_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_2d_image_with_variances_with_filename("val_and_var.pdf")


def test_plot_collapse_mpl():
    sc.plot_config.backend = "matplotlib"
    do_test_plot_collapse()


def test_plot_1d_mpl_as_keyword_arg():
    do_test_plot_1d(backend="matplotlib")
