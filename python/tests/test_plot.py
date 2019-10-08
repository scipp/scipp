# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import scipp as sp
import numpy as np
import io
from contextlib import redirect_stdout

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.


def do_plot(d, **kwargs):
    with io.StringIO() as buf, redirect_stdout(buf):
        sp.plot(d, **kwargs)
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
    d1 = sp.Dataset(
        coords={
            sp.Dim.X: sp.Variable([sp.Dim.X], values=xx, unit=sp.units.m),
            sp.Dim.Y: sp.Variable([sp.Dim.Y], values=yy, unit=sp.units.m)
        })
    params = {"values": a}
    if variances:
        params["variances"] = np.random.rand(M, N) + (x == y)
    d1["Sample"] = sp.Variable([sp.Dim.Y, sp.Dim.X],
                               unit=sp.units.counts,
                               **params)
    return d1


def do_test_plot_1d(**kwargs):
    d1 = sp.Dataset()
    N = 100
    d1.coords[sp.Dim.Tof] = sp.Variable([sp.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sp.units.us)
    d1["Sample"] = sp.Variable([sp.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               unit=sp.units.counts)
    do_plot(d1, **kwargs)


def do_test_plot_1d_with_variances():
    d1 = sp.Dataset()
    N = 100
    d1.coords[sp.Dim.Tof] = sp.Variable([sp.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sp.units.us)
    d1["Sample"] = sp.Variable([sp.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               variances=np.random.rand(N),
                               unit=sp.units.counts)
    do_plot(d1)


def do_test_plot_1d_bin_edges():
    d1 = sp.Dataset()
    N = 100
    d1.coords[sp.Dim.Tof] = sp.Variable([sp.Dim.Tof],
                                        values=np.arange(N + 1).astype(
                                            np.float64),
                                        unit=sp.units.us)
    d1["Sample"] = sp.Variable([sp.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               unit=sp.units.counts)
    do_plot(d1)


def do_test_plot_1d_bin_edges_with_variances():
    d1 = sp.Dataset()
    N = 100
    d1.coords[sp.Dim.Tof] = sp.Variable([sp.Dim.Tof],
                                        values=np.arange(N + 1).astype(
                                            np.float64),
                                        unit=sp.units.us)
    d1["Sample"] = sp.Variable([sp.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               variances=np.random.rand(N),
                               unit=sp.units.counts)
    do_plot(d1)


def do_test_plot_1d_two_entries():
    d1 = sp.Dataset()
    N = 100
    d1.coords[sp.Dim.Tof] = sp.Variable([sp.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sp.units.us)
    d1["Sample"] = sp.Variable([sp.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               unit=sp.units.counts)
    d1["Background"] = sp.Variable([sp.Dim.Tof],
                                   values=2.0 * np.random.rand(N),
                                   unit=sp.units.counts)
    do_plot(d1)


def do_test_plot_1d_list_of_datasets():
    N = 100
    d1 = sp.Dataset()
    d1.coords[sp.Dim.Tof] = sp.Variable([sp.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sp.units.us)
    d1["Sample"] = sp.Variable([sp.Dim.Tof], values=10.0 * np.random.rand(N))
    d1["Background"] = sp.Variable([sp.Dim.Tof],
                                   values=2.0 * np.random.rand(N))
    d2 = sp.Dataset()
    d2.coords[sp.Dim.Tof] = sp.Variable([sp.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sp.units.us)
    d2["Sample"] = sp.Variable([sp.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               variances=np.random.rand(N))
    d2["Background"] = sp.Variable([sp.Dim.Tof],
                                   values=2.0 * np.random.rand(N),
                                   variances=np.random.rand(N))
    do_plot([d1, d2])


def do_test_plot_2d_image():
    d1 = make_2d_dataset()
    do_plot(d1)


def do_test_plot_2d_image_with_axes():
    d1 = make_2d_dataset()
    do_plot(d1, axes=[sp.Dim.X, sp.Dim.Y])


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
    d1 = sp.Dataset()
    d1.coords[sp.Dim.Tof] = sp.Variable([sp.Dim.Tof],
                                        values=np.arange(N + 1).astype(
                                            np.float64),
                                        unit=sp.units.us)
    d1.coords[sp.Dim.X] = sp.Variable([sp.Dim.X],
                                      values=np.arange(M).astype(np.float64),
                                      unit=sp.units.m)
    d1["Sample"] = sp.Variable([sp.Dim.X, sp.Dim.Tof],
                               values=10.0 * np.random.rand(M, N),
                               variances=np.random.rand(M, N))
    do_plot(d1, collapse=sp.Dim.Tof)


def do_test_plot_sliceviewer():
    d1 = sp.Dataset()
    n1 = 20
    n2 = 30
    n3 = 40
    d1.coords[sp.Dim.X] = sp.Variable([sp.Dim.X],
                                      np.arange(n1).astype(np.float64))
    d1.coords[sp.Dim.Y] = sp.Variable([sp.Dim.Y],
                                      np.arange(n2).astype(np.float64))
    d1.coords[sp.Dim.Z] = sp.Variable([sp.Dim.Z],
                                      np.arange(n3).astype(np.float64))
    d1["Sample"] = sp.Variable([sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
                               values=np.arange(n1 * n2 * n3).reshape(
                                   n3, n2, n1).astype(np.float64))
    do_plot(d1)


def do_test_plot_sliceviewer_with_variances():
    d1 = sp.Dataset()
    n1 = 20
    n2 = 30
    n3 = 40
    d1.coords[sp.Dim.X] = sp.Variable([sp.Dim.X],
                                      np.arange(n1).astype(np.float64))
    d1.coords[sp.Dim.Y] = sp.Variable([sp.Dim.Y],
                                      np.arange(n2).astype(np.float64))
    d1.coords[sp.Dim.Z] = sp.Variable([sp.Dim.Z],
                                      np.arange(n3).astype(np.float64))
    a = np.arange(n1 * n2 * n3).reshape(n3, n2, n1).astype(np.float64)
    d1["Sample"] = sp.Variable([sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
                               values=a,
                               variances=np.random.rand(n3, n2, n1) * a * 0.1)
    do_plot(d1, show_variances=True)


def do_test_plot_sliceviewer_with_two_sliders():
    d1 = sp.Dataset()
    n1 = 20
    n2 = 30
    n3 = 40
    n4 = 50
    d1.coords[sp.Dim.X] = sp.Variable([sp.Dim.X],
                                      np.arange(n1).astype(np.float64))
    d1.coords[sp.Dim.Y] = sp.Variable([sp.Dim.Y],
                                      np.arange(n2).astype(np.float64))
    d1.coords[sp.Dim.Z] = sp.Variable([sp.Dim.Z],
                                      np.arange(n3).astype(np.float64))
    d1.coords[sp.Dim.Tof] = sp.Variable([sp.Dim.Tof],
                                        np.arange(n4).astype(np.float64))
    d1["Sample"] = sp.Variable([sp.Dim.Tof, sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
                               values=np.arange(n1 * n2 * n3 * n4).reshape(
                                   n4, n3, n2, n1).astype(np.float64))
    do_plot(d1)


def do_test_plot_sliceviewer_with_axes():
    d1 = sp.Dataset()
    n1 = 20
    n2 = 30
    n3 = 40
    d1.coords[sp.Dim.X] = sp.Variable([sp.Dim.X],
                                      np.arange(n1).astype(np.float64))
    d1.coords[sp.Dim.Y] = sp.Variable([sp.Dim.Y],
                                      np.arange(n2).astype(np.float64))
    d1.coords[sp.Dim.Z] = sp.Variable([sp.Dim.Z],
                                      np.arange(n3).astype(np.float64))
    d1["Sample"] = sp.Variable([sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
                               values=np.arange(n1 * n2 * n3).reshape(
                                   n3, n2, n1).astype(np.float64))
    do_plot(d1, axes=[sp.Dim.Y, sp.Dim.X, sp.Dim.Z])


def do_test_plot_sliceviewer_with_3d_projection():
    d1 = sp.Dataset()
    n1 = 20
    n2 = 30
    n3 = 40
    d1.coords[sp.Dim.X] = sp.Variable([sp.Dim.X],
                                      np.arange(n1).astype(np.float64))
    d1.coords[sp.Dim.Y] = sp.Variable([sp.Dim.Y],
                                      np.arange(n2).astype(np.float64))
    d1.coords[sp.Dim.Z] = sp.Variable([sp.Dim.Z],
                                      np.arange(n3).astype(np.float64))
    d1["Sample"] = sp.Variable([sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
                               values=np.arange(n1 * n2 * n3).reshape(
                                   n3, n2, n1).astype(np.float64))
    do_plot(d1, projection="3d")


def do_test_plot_sliceviewer_with_3d_projection_with_variances():
    d1 = sp.Dataset()
    n1 = 20
    n2 = 30
    n3 = 40
    d1.coords[sp.Dim.X] = sp.Variable([sp.Dim.X],
                                      np.arange(n1).astype(np.float64))
    d1.coords[sp.Dim.Y] = sp.Variable([sp.Dim.Y],
                                      np.arange(n2).astype(np.float64))
    d1.coords[sp.Dim.Z] = sp.Variable([sp.Dim.Z],
                                      np.arange(n3).astype(np.float64))
    a = np.arange(n1 * n2 * n3).reshape(n3, n2, n1).astype(np.float64)
    d1["Sample"] = sp.Variable([sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
                               values=a,
                               variances=np.random.rand(n3, n2, n1) * a * 0.1)
    do_plot(d1, projection="3d", show_variances=True)


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


def test_plot_2d_image_with_axes():
    do_test_plot_2d_image_with_axes()


def test_plot_2d_image_with_variances():
    do_test_plot_2d_image_with_variances()


def test_plot_2d_image_with_filename():
    do_test_plot_2d_image_with_filename("image.html")


def test_plot_2d_image_with_variances_with_filename():
    do_test_plot_2d_image_with_variances_with_filename("val_and_var.html")


def test_plot_collapse():
    do_test_plot_collapse()


def test_plot_sliceviewer():
    do_test_plot_sliceviewer()


def test_plot_sliceviewer_with_variances():
    do_test_plot_sliceviewer_with_variances()


def test_plot_sliceviewer_with_two_sliders():
    do_test_plot_sliceviewer_with_two_sliders()


def test_plot_sliceviewer_with_axes():
    do_test_plot_sliceviewer_with_axes()


def test_plot_sliceviewer_with_3d_projection():
    do_test_plot_sliceviewer_with_3d_projection()


def test_plot_sliceviewer_with_3d_projection_with_variances():
    do_test_plot_sliceviewer_with_3d_projection_with_variances()


# Using matplotlib backend ====================================================


def test_plot_1d_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_1d()


def test_plot_1d_with_variances_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_1d_with_variances()


def test_plot_1d_bin_edges_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_1d_bin_edges()


def test_plot_1d_bin_edges_with_variances_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_1d_bin_edges_with_variances()


def test_plot_1d_two_entries_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_1d_bin_edges_with_variances()


def test_plot_1d_list_of_datasets_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_1d_list_of_datasets()


def test_plot_2d_image_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_2d_image()


def test_plot_2d_image_with_axes_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_2d_image_with_axes()


def test_plot_2d_image_with_variances_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_2d_image_with_variances()


def test_plot_2d_image_with_filename_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_2d_image_with_filename("image.pdf")


def test_plot_2d_image_with_variances_with_filename_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_2d_image_with_variances_with_filename("val_and_var.pdf")


def test_plot_collapse_mpl():
    sp.plot_config.backend = "matplotlib"
    do_test_plot_collapse()


def test_plot_1d_mpl_as_keyword_arg():
    do_test_plot_1d(backend="matplotlib")
