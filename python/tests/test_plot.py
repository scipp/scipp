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


def do_plot(d, test_plotly_backend=True, test_mpl_backend=False, **kwargs):
    with io.StringIO() as buf, redirect_stdout(buf):
        if test_plotly_backend:
            sc.plot.plot(d, **kwargs)
        if test_mpl_backend:
            out = sc.plot.plot(d, backend="matplotlib", **kwargs)
            assert isinstance(out, dict)

        # TODO: The "static" backend is currently not tested as it requires
        # the plotly-orca package to be installed via conda.
        # sc.plot.plot(d, backend="static", **kwargs)

    return


def make_1d_dataset(variances=False, binedges=False, labels=False):
    N = 100
    d1 = sc.Dataset()
    d1.coords[sc.Dim.Tof] = sc.Variable(
        [sc.Dim.Tof], values=np.arange(N + binedges).astype(np.float64),
        unit=sc.units.us)
    d1["Sample"] = sc.Variable([sc.Dim.Tof],
                               values=10.0 * np.random.rand(N),
                               unit=sc.units.counts)
    if variances:
        d1["Sample"].variances = np.random.rand(N)
    if labels:
        d1.labels["somelabels"] = sc.Variable(
            [sc.Dim.Tof], values=np.linspace(101., 105., N), unit=sc.units.s)
    return d1


def make_2d_dataset(variances=False, binedges=False, labels=False):
    N = 100
    M = 50
    xx = np.arange(N, dtype=np.float64)
    yy = np.arange(M, dtype=np.float64)
    if binedges:
        x, y = np.meshgrid(xx[:-1], yy[:-1])
    else:
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
        params["variances"] = np.random.normal(a * 0.1, 0.05)
    d1["Sample"] = sc.Variable([sc.Dim.Y, sc.Dim.X],
                               unit=sc.units.counts,
                               **params)
    if labels:
        d1.labels["somelabels"] = sc.Variable(
            [sc.Dim.X], values=np.linspace(101., 105., N), unit=sc.units.s)
    return d1


def make_3d_dataset(variances=False, binedges=False, labels=False):
    n1 = 20
    n2 = 30
    n3 = 40
    d1 = sc.Dataset()
    d1.coords[sc.Dim.X] = sc.Variable(
        [sc.Dim.X], np.arange(n1 + binedges).astype(np.float64))
    d1.coords[sc.Dim.Y] = sc.Variable(
        [sc.Dim.Y], np.arange(n2 + binedges).astype(np.float64))
    d1.coords[sc.Dim.Z] = sc.Variable(
        [sc.Dim.Z], np.arange(n3 + binedges).astype(np.float64))
    a = np.arange(n1 * n2 * n3).reshape(n3, n2, n1).astype(np.float64)
    d1["Sample"] = sc.Variable([sc.Dim.Z, sc.Dim.Y, sc.Dim.X], values=a)
    if variances:
        d1["Sample"].variances = np.random.normal(a * 0.1, 0.05)
    if labels:
        d1.labels["somelabels"] = sc.Variable(
            [sc.Dim.Y], values=np.linspace(101., 105., n2), unit=sc.units.s)
    return d1


def test_plot_1d():
    d1 = make_1d_dataset()
    do_plot(d1, test_mpl_backend=True)


def test_plot_1d_with_variances():
    d1 = make_1d_dataset(variances=True)
    do_plot(d1, test_mpl_backend=True)


def test_plot_1d_bin_edges():
    d1 = make_1d_dataset(binedges=True)
    do_plot(d1, test_mpl_backend=True)


def test_plot_1d_with_labels():
    d1 = make_1d_dataset(labels=True)
    do_plot(d1, axes="somelabels", test_mpl_backend=True)


def test_plot_1d_log_axes():
    d1 = make_1d_dataset()
    do_plot(d1, logx=True, test_mpl_backend=True)
    do_plot(d1, logy=True, test_mpl_backend=True)
    do_plot(d1, logxy=True, test_mpl_backend=True)


def test_plot_1d_bin_edges_with_variances():
    d1 = make_1d_dataset(variances=True, binedges=True)
    do_plot(d1, test_mpl_backend=True)


def test_plot_1d_two_entries():
    d1 = make_1d_dataset()
    d1["Background"] = sc.Variable([sc.Dim.Tof],
                                   values=2.0 * np.random.rand(100),
                                   unit=sc.units.counts)
    do_plot(d1, test_mpl_backend=True)


def test_plot_1d_three_entries_with_labels():
    N = 100
    d1 = make_1d_dataset(labels=True)
    d1["Background"] = sc.Variable([sc.Dim.Tof],
                                   values=2.0 * np.random.rand(N),
                                   unit=sc.units.counts)
    d1.coords[sc.Dim.X] = sc.Variable([sc.Dim.X],
                                      values=np.arange(N).astype(np.float64),
                                      unit=sc.units.m)
    d1["Sample2"] = sc.Variable([sc.Dim.X],
                                values=10.0 * np.random.rand(N),
                                unit=sc.units.counts)
    d1.labels["Xlabels"] = sc.Variable([sc.Dim.X],
                                       values=np.linspace(151., 155., N),
                                       unit=sc.units.s)
    do_plot(d1, axes={sc.Dim.X: "Xlabels", sc.Dim.Tof: "somelabels"},
            test_mpl_backend=True)


def test_plot_1d_list_of_datasets():
    d1 = make_1d_dataset(binedges=True)
    d2 = make_1d_dataset(binedges=True, variances=True)
    do_plot([d1, d2], test_mpl_backend=True)


def test_plot_2d_image():
    d1 = make_2d_dataset()
    do_plot(d1, test_mpl_backend=True)


def test_plot_2d_image_with_axes():
    d1 = make_2d_dataset()
    do_plot(d1, axes=[sc.Dim.X, sc.Dim.Y], test_mpl_backend=True)


def test_plot_2d_image_with_labels():
    d1 = make_2d_dataset(labels=True)
    do_plot(d1, axes=[sc.Dim.Y, "somelabels"], test_mpl_backend=True)


def test_plot_2d_image_with_variances():
    d1 = make_2d_dataset(variances=True)
    do_plot(d1, show_variances=True)


def test_plot_2d_image_with_variances_for_mpl():
    d1 = make_2d_dataset(variances=True)
    do_plot(d1, test_plotly_backend=False, test_mpl_backend=True,
            variances=True)


def test_plot_2d_image_with_filename():
    d1 = make_2d_dataset()
    do_plot(d1, filename="image.html")


def test_plot_2d_image_with_variances_with_filename():
    d1 = make_2d_dataset(variances=True)
    do_plot(d1, show_variances=True, filename="val_and_var.html")


def test_plot_2d_image_with_bin_edges():
    d1 = make_2d_dataset(binedges=True)
    do_plot(d1, test_mpl_backend=True)


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
    do_plot(d1, collapse=sc.Dim.Tof, test_mpl_backend=True)


def test_plot_sliceviewer():
    d1 = make_3d_dataset()
    do_plot(d1)


def test_plot_sliceviewer_with_variances():
    d1 = make_3d_dataset(variances=True)
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
    d1 = make_3d_dataset()
    do_plot(d1, axes=[sc.Dim.Y, sc.Dim.X, sc.Dim.Z])


def test_plot_sliceviewer_with_labels():
    d1 = make_3d_dataset(labels=True)
    do_plot(d1, axes=[sc.Dim.X, sc.Dim.Z, "somelabels"])


def test_plot_sliceviewer_with_labels_bad_dimension():
    d1 = make_3d_dataset(labels=True)
    with pytest.raises(Exception) as e:
        do_plot(d1, axes=[sc.Dim.Y, sc.Dim.Z, "somelabels"])
    assert str(e.value) == ("The dimension of the labels cannot also be "
                            "specified as another axis.")


def test_plot_sliceviewer_with_3d_projection():
    d1 = make_3d_dataset()
    do_plot(d1, projection="3d")


def test_plot_sliceviewer_with_3d_projection_with_variances():
    d1 = make_3d_dataset(variances=True)
    do_plot(d1, projection="3d", show_variances=True)


def test_plot_sliceviewer_with_3d_projection_with_labels():
    d1 = make_3d_dataset(labels=True)
    do_plot(d1, projection="3d", axes=[sc.Dim.X, sc.Dim.Z, "somelabels"])


def test_plot_2d_image_rasterized():
    d1 = make_2d_dataset()
    do_plot(d1, rasterize=True)


def test_plot_2d_image_with_variances_rasterized():
    d1 = make_2d_dataset(variances=True)
    do_plot(d1, show_variances=True, rasterize=True)


def test_plot_sliceviewer_rasterized():
    d1 = make_3d_dataset()
    do_plot(d1, rasterize=True)
