# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import scipp as sc
import numpy as np
import io
from contextlib import redirect_stdout

# TODO: For now we are just checking that the plot does not throw any errors.
# In the future it would be nice to check the output by either comparing
# checksums or by using tools like squish.


def do_plot(d, **kwargs):
    with io.StringIO() as buf, redirect_stdout(buf):
        sc.plot.plot(d, **kwargs)

        # TODO: The "static" backend is currently not tested as it requires
        # the plotly-orca package to be installed via conda.
        # sc.plot.plot(d, backend="static", **kwargs)

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


def test_plot_1d():
    d1 = sc.Dataset()
    N = 100
    d1.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                        values=np.arange(N).astype(np.float64),
                                        unit=sc.units.us)
    d1["Sample"] = sc.Variable([sc.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               unit=sc.units.counts)
    do_plot(d1)


def test_plot_1d_with_variances():
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


def test_plot_1d_bin_edges():
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


def test_plot_1d_bin_edges_with_variances():
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


def test_plot_1d_two_entries():
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


def test_plot_1d_list_of_datasets():
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


def test_plot_2d_image():
    d1 = make_2d_dataset()
    do_plot(d1)


def test_plot_2d_image_with_axes():
    d1 = make_2d_dataset()
    do_plot(d1, axes=[sc.Dim.X, sc.Dim.Y])


def test_plot_2d_image_with_variances():
    d1 = make_2d_dataset(variances=True)
    do_plot(d1)


def test_plot_2d_image_with_filename():
    d1 = make_2d_dataset()
    do_plot(d1, filename="image.html")


def test_plot_2d_image_with_variances_with_filename():
    d1 = make_2d_dataset(variances=True)
    do_plot(d1, filename="val_and_var.html")


def test_plot_collapse():
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


def test_plot_sliceviewer():
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


def test_plot_sliceviewer_with_variances():
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
    a = np.arange(n1 * n2 * n3).reshape(n3, n2, n1).astype(np.float64)
    d1["Sample"] = sc.Variable([sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
                               values=a,
                               variances=np.random.rand(n3, n2, n1) * a * 0.1)
    do_plot(d1, show_variances=True)


def test_plot_sliceviewer_with_two_sliders():
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


def test_plot_sliceviewer_with_axes():
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


def test_plot_sliceviewer_with_3d_projection():
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
    do_plot(d1, projection="3d")


def test_plot_sliceviewer_with_3d_projection_with_variances():
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
    a = np.arange(n1 * n2 * n3).reshape(n3, n2, n1).astype(np.float64)
    d1["Sample"] = sc.Variable([sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
                               values=a,
                               variances=np.random.rand(n3, n2, n1) * a * 0.1)
    do_plot(d1, projection="3d", show_variances=True)


def test_plot_2d_image_rasterized():
    d1 = make_2d_dataset()
    do_plot(d1, rasterize=True)


def test_plot_2d_image_with_variances_rasterized():
    d1 = make_2d_dataset(variances=True)
    do_plot(d1, rasterize=True)
