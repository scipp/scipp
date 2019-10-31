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
    d = sc.Dataset()
    d.coords[sc.Dim.Tof] = sc.Variable(
        [sc.Dim.Tof], values=np.arange(N + binedges).astype(np.float64),
        unit=sc.units.us)
    d["Sample"] = sc.Variable([sc.Dim.Tof],
                              values=10.0 * np.random.rand(N),
                              unit=sc.units.counts)
    if variances:
        d["Sample"].variances = np.random.rand(N)
    if labels:
        d.labels["somelabels"] = sc.Variable(
            [sc.Dim.Tof], values=np.linspace(101., 105., N), unit=sc.units.s)
    return d


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
    d = sc.Dataset(
        coords={
            sc.Dim.X: sc.Variable([sc.Dim.X], values=xx, unit=sc.units.m),
            sc.Dim.Y: sc.Variable([sc.Dim.Y], values=yy, unit=sc.units.m)
        })
    params = {"values": a}
    if variances:
        params["variances"] = np.random.normal(a * 0.1, 0.05)
    d["Sample"] = sc.Variable([sc.Dim.Y, sc.Dim.X],
                              unit=sc.units.counts,
                              **params)
    if labels:
        d.labels["somelabels"] = sc.Variable(
            [sc.Dim.X], values=np.linspace(101., 105., N), unit=sc.units.s)
    return d


def make_3d_dataset(variances=False, binedges=False, labels=False):
    n1 = 20
    n2 = 30
    n3 = 40
    d = sc.Dataset()
    d.coords[sc.Dim.X] = sc.Variable(
        [sc.Dim.X], np.arange(n1 + binedges).astype(np.float64))
    d.coords[sc.Dim.Y] = sc.Variable(
        [sc.Dim.Y], np.arange(n2 + binedges).astype(np.float64))
    d.coords[sc.Dim.Z] = sc.Variable(
        [sc.Dim.Z], np.arange(n3 + binedges).astype(np.float64))
    a = np.arange(n1 * n2 * n3).reshape(n3, n2, n1).astype(np.float64)
    d["Sample"] = sc.Variable([sc.Dim.Z, sc.Dim.Y, sc.Dim.X], values=a)
    if variances:
        d["Sample"].variances = np.random.normal(a * 0.1, 0.05)
    if labels:
        d.labels["somelabels"] = sc.Variable(
            [sc.Dim.Y], values=np.linspace(101., 105., n2), unit=sc.units.s)
    return d


def make_1d_sparse_dataset(data=False):
    N = 50
    var = sc.Variable(dims=[sc.Dim.Tof],
                      shape=[sc.Dimensions.Sparse],
                      unit=sc.units.us)
    v = np.random.normal(50.0, scale=20.0, size=N)
    var.values = v
    d = sc.Dataset()
    if data:
        dat = sc.Variable(dims=[sc.Dim.Tof],
                          shape=[sc.Dimensions.Sparse],
                          unit=sc.units.us)
        dat.values = v * 0.5
        d['a'] = sc.DataArray(data=dat, coords={sc.Dim.Tof: var})
    else:
        d['a'] = sc.DataArray(coords={sc.Dim.Tof: var})
    return d


def make_2d_sparse_dataset(data=False):
    N = 50
    M = 10
    var = sc.Variable(dims=[sc.Dim.X, sc.Dim.Tof],
                      shape=[M, sc.Dimensions.Sparse],
                      unit=sc.units.us)
    if data:
        dat = sc.Variable(dims=[sc.Dim.X, sc.Dim.Tof],
                          shape=[M, sc.Dimensions.Sparse],
                          unit=sc.units.us)
    for i in range(M):
        v = np.random.normal(50.0, scale=20.0, size=int(np.random.rand()*N))
        var[sc.Dim.X, i].values = v
        if data:
            dat[sc.Dim.X, i].values = v * 0.5

    d = sc.Dataset()
    d.coords[sc.Dim.X] = sc.Variable([sc.Dim.X], values=np.arange(M),
                                     unit=sc.units.m)
    if data:
        d['a'] = sc.DataArray(data=dat, coords={sc.Dim.Tof: var})
    else:
        d['a'] = sc.DataArray(coords={sc.Dim.Tof: var})
    return d


def make_3d_sparse_dataset(data=False):
    N = 50
    M = 10
    L = 15
    var = sc.Variable(dims=[sc.Dim.Y, sc.Dim.X, sc.Dim.Tof],
                      shape=[L, M, sc.Dimensions.Sparse],
                      unit=sc.units.us)
    if data:
        dat = sc.Variable(dims=[sc.Dim.Y, sc.Dim.X, sc.Dim.Tof],
                          shape=[L, M, sc.Dimensions.Sparse],
                          unit=sc.units.us)
    for i in range(M):
        for j in range(L):
            v = np.random.normal(50.0, scale=20.0,
                                 size=int(np.random.rand()*N))
            var[sc.Dim.Y, j][sc.Dim.X, i].values = v
            if data:
                dat[sc.Dim.Y, j][sc.Dim.X, i].values = v * 0.5

    d = sc.Dataset()
    d.coords[sc.Dim.X] = sc.Variable([sc.Dim.X], values=np.arange(M),
                                     unit=sc.units.m)
    d.coords[sc.Dim.Y] = sc.Variable([sc.Dim.Y], values=np.arange(L),
                                     unit=sc.units.m)
    if data:
        d['a'] = sc.DataArray(data=dat, coords={sc.Dim.Tof: var})
    else:
        d['a'] = sc.DataArray(coords={sc.Dim.Tof: var})
    return d


def test_plot_1d():
    d = make_1d_dataset()
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_with_variances():
    d = make_1d_dataset(variances=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_bin_edges():
    d = make_1d_dataset(binedges=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_with_labels():
    d = make_1d_dataset(labels=True)
    do_plot(d, axes="somelabels", test_mpl_backend=True)


def test_plot_1d_log_axes():
    d = make_1d_dataset()
    do_plot(d, logx=True, test_mpl_backend=True)
    do_plot(d, logy=True, test_mpl_backend=True)
    do_plot(d, logxy=True, test_mpl_backend=True)


def test_plot_1d_bin_edges_with_variances():
    d = make_1d_dataset(variances=True, binedges=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_two_entries():
    d = make_1d_dataset()
    d["Background"] = sc.Variable([sc.Dim.Tof],
                                  values=2.0 * np.random.rand(100),
                                  unit=sc.units.counts)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_three_entries_with_labels():
    N = 100
    d = make_1d_dataset(labels=True)
    d["Background"] = sc.Variable([sc.Dim.Tof],
                                  values=2.0 * np.random.rand(N),
                                  unit=sc.units.counts)
    d.coords[sc.Dim.X] = sc.Variable([sc.Dim.X],
                                     values=np.arange(N).astype(np.float64),
                                     unit=sc.units.m)
    d["Sample2"] = sc.Variable([sc.Dim.X],
                               values=10.0 * np.random.rand(N),
                               unit=sc.units.counts)
    d.labels["Xlabels"] = sc.Variable([sc.Dim.X],
                                      values=np.linspace(151., 155., N),
                                      unit=sc.units.s)
    do_plot(d, axes={sc.Dim.X: "Xlabels", sc.Dim.Tof: "somelabels"},
            test_mpl_backend=True)


def test_plot_2d_image():
    d = make_2d_dataset()
    do_plot(d, test_mpl_backend=True)


def test_plot_2d_image_with_axes():
    d = make_2d_dataset()
    do_plot(d, axes=[sc.Dim.X, sc.Dim.Y], test_mpl_backend=True)


def test_plot_2d_image_with_labels():
    d = make_2d_dataset(labels=True)
    do_plot(d, axes=[sc.Dim.Y, "somelabels"], test_mpl_backend=True)


def test_plot_2d_image_with_variances():
    d = make_2d_dataset(variances=True)
    do_plot(d, show_variances=True)


def test_plot_2d_image_with_variances_for_mpl():
    d = make_2d_dataset(variances=True)
    do_plot(d, test_plotly_backend=False, test_mpl_backend=True,
            variances=True)


def test_plot_2d_image_with_filename():
    d = make_2d_dataset()
    do_plot(d, filename="image.html")


def test_plot_2d_image_with_variances_with_filename():
    d = make_2d_dataset(variances=True)
    do_plot(d, show_variances=True, filename="val_and_var.html")


def test_plot_2d_image_with_bin_edges():
    d = make_2d_dataset(binedges=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_collapse():
    N = 100
    M = 5
    d = sc.Dataset()
    d.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                       values=np.arange(N + 1).astype(
                                       np.float64),
                                       unit=sc.units.us)
    d.coords[sc.Dim.X] = sc.Variable([sc.Dim.X],
                                     values=np.arange(M).astype(np.float64),
                                     unit=sc.units.m)
    d["Sample"] = sc.Variable([sc.Dim.X, sc.Dim.Tof],
                              values=10.0 * np.random.rand(M, N),
                              variances=np.random.rand(M, N))
    do_plot(d, collapse=sc.Dim.Tof, test_mpl_backend=True)


def test_plot_sliceviewer():
    d = make_3d_dataset()
    do_plot(d)


def test_plot_sliceviewer_with_variances():
    d = make_3d_dataset(variances=True)
    do_plot(d, show_variances=True)


def test_plot_sliceviewer_with_two_sliders():
    d = sc.Dataset()
    n1 = 20
    n2 = 30
    n3 = 40
    n4 = 50
    d.coords[sc.Dim.X] = sc.Variable([sc.Dim.X],
                                     np.arange(n1).astype(np.float64))
    d.coords[sc.Dim.Y] = sc.Variable([sc.Dim.Y],
                                     np.arange(n2).astype(np.float64))
    d.coords[sc.Dim.Z] = sc.Variable([sc.Dim.Z],
                                     np.arange(n3).astype(np.float64))
    d.coords[sc.Dim.Tof] = sc.Variable([sc.Dim.Tof],
                                       np.arange(n4).astype(np.float64))
    d["Sample"] = sc.Variable([sc.Dim.Tof, sc.Dim.Z, sc.Dim.Y, sc.Dim.X],
                              values=np.arange(n1 * n2 * n3 * n4).reshape(
                                  n4, n3, n2, n1).astype(np.float64))
    do_plot(d)


def test_plot_sliceviewer_with_axes():
    d = make_3d_dataset()
    do_plot(d, axes=[sc.Dim.Y, sc.Dim.X, sc.Dim.Z])


def test_plot_sliceviewer_with_labels():
    d = make_3d_dataset(labels=True)
    do_plot(d, axes=[sc.Dim.X, sc.Dim.Z, "somelabels"])


def test_plot_sliceviewer_with_labels_bad_dimension():
    d = make_3d_dataset(labels=True)
    with pytest.raises(Exception) as e:
        do_plot(d, axes=[sc.Dim.Y, sc.Dim.Z, "somelabels"])
    assert str(e.value) == ("The dimension of the labels cannot also be "
                            "specified as another axis.")


def test_plot_sliceviewer_with_3d_projection():
    d = make_3d_dataset()
    do_plot(d, projection="3d")


def test_plot_sliceviewer_with_3d_projection_with_variances():
    d = make_3d_dataset(variances=True)
    do_plot(d, projection="3d", show_variances=True)


def test_plot_sliceviewer_with_3d_projection_with_labels():
    d = make_3d_dataset(labels=True)
    do_plot(d, projection="3d", axes=[sc.Dim.X, sc.Dim.Z, "somelabels"])


def test_plot_2d_image_rasterized():
    d = make_2d_dataset()
    do_plot(d, rasterize=True)


def test_plot_2d_image_with_variances_rasterized():
    d = make_2d_dataset(variances=True)
    do_plot(d, show_variances=True, rasterize=True)


def test_plot_sliceviewer_rasterized():
    d = make_3d_dataset()
    do_plot(d, rasterize=True)


def test_plot_1d_sparse_data():
    d = make_1d_sparse_dataset()
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_sparse_data_with_weights():
    d = make_1d_sparse_dataset(data=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_sparse_data_with_int_bins():
    d = make_1d_sparse_dataset()
    do_plot(d, bins=50, test_mpl_backend=True)


def test_plot_1d_sparse_data_with_nparray_bins():
    d = make_1d_sparse_dataset()
    do_plot(d, bins=np.linspace(0.0, 105.0, 50), test_mpl_backend=True)


def test_plot_1d_sparse_data_with_Variable_bins():
    d = make_1d_sparse_dataset()
    bins = sc.Variable([sc.Dim.Tof],
                       values=np.linspace(0.0, 105.0, 50),
                       unit=sc.units.us)
    do_plot(d, bins=bins, test_mpl_backend=True)


def test_plot_2d_sparse_data():
    d = make_2d_sparse_dataset()
    do_plot(d, test_mpl_backend=True)


def test_plot_2d_sparse_data_with_weights():
    d = make_2d_sparse_dataset(data=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_2d_sparse_data_with_int_bins():
    d = make_2d_sparse_dataset()
    do_plot(d, bins=50, test_mpl_backend=True)


def test_plot_2d_sparse_data_with_nparray_bins():
    d = make_2d_sparse_dataset()
    do_plot(d, bins=np.linspace(0.0, 105.0, 50), test_mpl_backend=True)


def test_plot_2d_sparse_data_with_Variable_bins():
    d = make_2d_sparse_dataset()
    bins = sc.Variable([sc.Dim.Tof],
                       values=np.linspace(0.0, 105.0, 50),
                       unit=sc.units.us)
    do_plot(d, bins=bins, test_mpl_backend=True)


def test_plot_3d_sparse_data():
    d = make_3d_sparse_dataset()
    do_plot(d, test_mpl_backend=True)


def test_plot_3d_sparse_data_with_weights():
    d = make_3d_sparse_dataset(data=True)
    do_plot(d, test_mpl_backend=True)


@pytest.mark.skip(reason="RuntimeError: Only the simple case histograms may "
                         "be constructed for now: 2 dims including sparse.")
def test_plot_3d_sparse_data_with_int_bins():
    d = make_3d_sparse_dataset()
    do_plot(d, bins=50, test_mpl_backend=True)


@pytest.mark.skip(reason="RuntimeError: Only the simple case histograms may "
                         "be constructed for now: 2 dims including sparse.")
def test_plot_3d_sparse_data_with_nparray_bins():
    d = make_3d_sparse_dataset()
    do_plot(d, bins=np.linspace(0.0, 105.0, 50), test_mpl_backend=True)


@pytest.mark.skip(reason="RuntimeError: Only the simple case histograms may "
                         "be constructed for now: 2 dims including sparse.")
def test_plot_3d_sparse_data_with_Variable_bins():
    d = make_3d_sparse_dataset()
    bins = sc.Variable([sc.Dim.Tof],
                       values=np.linspace(0.0, 105.0, 50),
                       unit=sc.units.us)
    do_plot(d, bins=bins, test_mpl_backend=True)
