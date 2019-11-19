# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import scipp as sc
import numpy as np
import io
from contextlib import redirect_stdout
from itertools import product
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


def make_dense_dataset(ndim=1, variances=False, binedges=False, labels=False):

    dim_list = [sc.Dim.Tof, sc.Dim.X, sc.Dim.Y, sc.Dim.Z, sc.Dim.Qx]

    N = 50
    M = 10

    d = sc.Dataset()
    shapes = []
    dims = []
    for i in range(ndim):
        n = N - (i*M)
        d.coords[dim_list[i]] = sc.Variable(
            [dim_list[i]], np.arange(n + binedges).astype(np.float64))
        dims.append(dim_list[i])
        shapes.append(n)
    a = np.arange(np.prod(shapes)).reshape(*shapes).astype(np.float64)
    d["Sample"] = sc.Variable(dims, values=a)
    if variances:
        d["Sample"].variances = np.abs(np.random.normal(a * 0.1, 0.05))
    if labels:
        d.labels["somelabels"] = sc.Variable(
            [dim_list[0]], values=np.linspace(101., 105., shapes[0]),
            unit=sc.units.s)
    return d


def make_sparse_dataset(ndim=1, data=False):

    dim_list = [sc.Dim.Tof, sc.Dim.X, sc.Dim.Y, sc.Dim.Z, sc.Dim.Qx]

    N = 50
    M = 10

    dims = []
    shapes = []
    for i in range(1, ndim):
        n = N - (i*M)
        dims.append(dim_list[i])
        shapes.append(n)
    dims.append(dim_list[0])
    shapes.append(sc.Dimensions.Sparse)

    var = sc.Variable(dims=dims, shape=shapes, unit=sc.units.us)
    if data:
        dat = sc.Variable(dims=dims, shape=shapes, unit=sc.units.us)

    if ndim > 1:
        indices = tuple()
        for i in range(ndim - 1):
            indices += range(shapes[i]),
    else:
        indices = [0],
    # Now construct all indices combinations using itertools
    for ind in product(*indices):
        # And for each indices combination, slice the original
        # data down to the sparse dimension
        vslice = var
        if data:
            dslice = dat
        if ndim > 1:
            for i in range(ndim - 1):
                vslice = vslice[dims[i], ind[i]]
                if data:
                    dslice = dslice[dims[i], ind[i]]
        v = np.random.normal(float(N), scale=2.0*M,
                             size=int(np.random.rand()*N))
        vslice.values = v
        if data:
            dslice.values = v * 0.5

    d = sc.Dataset()
    for i in range(1, ndim):
        d.coords[dim_list[i]] = sc.Variable(
            [dim_list[i]], values=np.arange(N - (i*M), dtype=np.float),
            unit=sc.units.m)
    params = {"coords": {dim_list[0]: var}}
    if data:
        params["data"] = dat
    d['a'] = sc.DataArray(**params)
    return d


def test_plot_1d():
    d = make_dense_dataset(ndim=1)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_with_variances():
    d = make_dense_dataset(ndim=1, variances=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_bin_edges():
    d = make_dense_dataset(ndim=1, binedges=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_with_labels():
    d = make_dense_dataset(ndim=1, labels=True)
    do_plot(d, axes=["somelabels"], test_mpl_backend=True)


def test_plot_1d_log_axes():
    d = make_dense_dataset(ndim=1)
    do_plot(d, logx=True, test_mpl_backend=True)
    do_plot(d, logy=True, test_mpl_backend=True)
    do_plot(d, logxy=True, test_mpl_backend=True)


def test_plot_1d_bin_edges_with_variances():
    d = make_dense_dataset(ndim=1, variances=True, binedges=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_two_entries():
    d = make_dense_dataset(ndim=1)
    d["Background"] = sc.Variable([sc.Dim.Tof],
                                  values=2.0 * np.random.rand(50),
                                  unit=sc.units.counts)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_three_entries_with_labels():
    N = 50
    d = make_dense_dataset(ndim=1, labels=True)
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
    d = make_dense_dataset(ndim=2)
    do_plot(d, test_mpl_backend=True)


def test_plot_2d_image_with_axes():
    d = make_dense_dataset(ndim=2)
    do_plot(d, axes=[sc.Dim.Tof, sc.Dim.X], test_mpl_backend=True)


def test_plot_2d_image_with_labels():
    d = make_dense_dataset(ndim=2, labels=True)
    do_plot(d, axes=[sc.Dim.X, "somelabels"], test_mpl_backend=True)


def test_plot_2d_image_with_variances():
    d = make_dense_dataset(ndim=2, variances=True)
    do_plot(d, show_variances=True, test_mpl_backend=True)


def test_plot_2d_image_with_filename():
    d = make_dense_dataset(ndim=2)
    do_plot(d, filename="image.html")


def test_plot_2d_image_with_variances_with_filename():
    d = make_dense_dataset(ndim=2, variances=True)
    do_plot(d, show_variances=True, filename="val_and_var.html")


def test_plot_2d_image_with_bin_edges():
    d = make_dense_dataset(ndim=2, binedges=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_collapse():
    d = make_dense_dataset(ndim=2)
    do_plot(d, collapse=sc.Dim.Tof, test_mpl_backend=True)


def test_plot_sliceviewer():
    d = make_dense_dataset(ndim=3)
    do_plot(d)


def test_plot_sliceviewer_with_variances():
    d = make_dense_dataset(ndim=3, variances=True)
    do_plot(d, show_variances=True)


def test_plot_sliceviewer_with_two_sliders():
    d = make_dense_dataset(ndim=4)
    do_plot(d)


def test_plot_sliceviewer_with_axes():
    d = make_dense_dataset(ndim=3)
    do_plot(d, axes=[sc.Dim.X, sc.Dim.Tof, sc.Dim.Y])


def test_plot_sliceviewer_with_labels():
    d = make_dense_dataset(ndim=3, labels=True)
    do_plot(d, axes=[sc.Dim.X, sc.Dim.Y, "somelabels"])


def test_plot_sliceviewer_with_labels_bad_dimension():
    d = make_dense_dataset(ndim=3, labels=True)
    with pytest.raises(Exception) as e:
        do_plot(d, axes=[sc.Dim.Tof, sc.Dim.Y, "somelabels"])
    assert str(e.value) == ("The dimension of the labels cannot also be "
                            "specified as another axis.")


def test_plot_sliceviewer_with_3d_projection():
    d = make_dense_dataset(ndim=3)
    do_plot(d, projection="3d")


def test_plot_sliceviewer_with_3d_projection_with_variances():
    d = make_dense_dataset(ndim=3, variances=True)
    do_plot(d, projection="3d", show_variances=True)


def test_plot_sliceviewer_with_3d_projection_with_labels():
    d = make_dense_dataset(ndim=3, labels=True)
    do_plot(d, projection="3d", axes=[sc.Dim.X, sc.Dim.Y, "somelabels"])


def test_plot_2d_image_rasterized():
    d = make_dense_dataset(ndim=2)
    do_plot(d, rasterize=True)


def test_plot_2d_image_with_variances_rasterized():
    d = make_dense_dataset(ndim=2, variances=True)
    do_plot(d, show_variances=True, rasterize=True)


def test_plot_sliceviewer_rasterized():
    d = make_dense_dataset(ndim=3)
    do_plot(d, rasterize=True)


def test_plot_1d_sparse_data():
    d = make_sparse_dataset(ndim=1)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_sparse_data_with_weights():
    d = make_sparse_dataset(ndim=1, data=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_1d_sparse_data_with_int_bins():
    d = make_sparse_dataset(ndim=1)
    do_plot(d, bins=50, test_mpl_backend=True)


def test_plot_1d_sparse_data_with_nparray_bins():
    d = make_sparse_dataset(ndim=1)
    do_plot(d, bins=np.linspace(0.0, 105.0, 50), test_mpl_backend=True)


def test_plot_1d_sparse_data_with_Variable_bins():
    d = make_sparse_dataset(ndim=1)
    bins = sc.Variable([sc.Dim.Tof],
                       values=np.linspace(0.0, 105.0, 50),
                       unit=sc.units.us)
    do_plot(d, bins=bins, test_mpl_backend=True)


def test_plot_2d_sparse_data():
    d = make_sparse_dataset(ndim=2)
    do_plot(d, test_mpl_backend=True)


def test_plot_2d_sparse_data_with_weights():
    d = make_sparse_dataset(ndim=2, data=True)
    do_plot(d, test_mpl_backend=True)


def test_plot_2d_sparse_data_with_int_bins():
    d = make_sparse_dataset(ndim=2)
    do_plot(d, bins=50, test_mpl_backend=True)


def test_plot_2d_sparse_data_with_nparray_bins():
    d = make_sparse_dataset(ndim=2)
    do_plot(d, bins=np.linspace(0.0, 105.0, 50), test_mpl_backend=True)


def test_plot_2d_sparse_data_with_Variable_bins():
    d = make_sparse_dataset(ndim=2)
    bins = sc.Variable([sc.Dim.Tof],
                       values=np.linspace(0.0, 105.0, 50),
                       unit=sc.units.us)
    do_plot(d, bins=bins, test_mpl_backend=True)


def test_plot_3d_sparse_data():
    d = make_sparse_dataset(ndim=3)
    do_plot(d, test_mpl_backend=True)


def test_plot_3d_sparse_data_with_weights():
    d = make_sparse_dataset(ndim=3, data=True)
    do_plot(d, test_mpl_backend=True)


@pytest.mark.skip(reason="RuntimeError: Only the simple case histograms may "
                         "be constructed for now: 2 dims including sparse.")
def test_plot_3d_sparse_data_with_int_bins():
    d = make_sparse_dataset(ndim=3)
    do_plot(d, bins=50, test_mpl_backend=True)


@pytest.mark.skip(reason="RuntimeError: Only the simple case histograms may "
                         "be constructed for now: 2 dims including sparse.")
def test_plot_3d_sparse_data_with_nparray_bins():
    d = make_sparse_dataset(ndim=3)
    do_plot(d, bins=np.linspace(0.0, 105.0, 50), test_mpl_backend=True)


@pytest.mark.skip(reason="RuntimeError: Only the simple case histograms may "
                         "be constructed for now: 2 dims including sparse.")
def test_plot_3d_sparse_data_with_Variable_bins():
    d = make_sparse_dataset(ndim=3)
    bins = sc.Variable([sc.Dim.Tof],
                       values=np.linspace(0.0, 105.0, 50),
                       unit=sc.units.us)
    do_plot(d, bins=bins, test_mpl_backend=True)
