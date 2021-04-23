# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file

import scipp as sc
from scipp.plotting.helpers import PlotArray
from plot_helper import make_dense_dataset


def assert_identical(plotarray, dataarray):
    assert sc.identical(plotarray.data, dataarray.data)
    for key in dataarray.meta:
        assert sc.identical(plotarray.meta[str(key)], dataarray.meta[key])
    for msk in dataarray.masks:
        assert sc.identical(plotarray.masks[msk], dataarray.masks[msk])


def test_plot_array_creation():
    da = make_dense_dataset(ndim=3, attrs=True, masks=True)["Sample"]
    pa = PlotArray(data=da.data, meta=da.meta, masks=da.masks)
    assert_identical(pa, da)


def test_plot_array_view_index():
    da = make_dense_dataset(ndim=3)["Sample"]
    pa = PlotArray(data=da.data, meta=da.coords)
    assert_identical(pa['x', 10], da['x', 10])


def test_plot_array_view_range():
    da = make_dense_dataset(ndim=3)["Sample"]
    pa = PlotArray(data=da.data, meta=da.coords)
    assert_identical(pa['y', 5:10], da['y', 5:10])


def test_plot_array_view_index_with_masks():
    da = make_dense_dataset(ndim=3, masks=True)["Sample"]
    pa = PlotArray(data=da.data, meta=da.coords, masks=da.masks)
    assert_identical(pa['x', 10], da['x', 10])


def test_plot_array_view_index_with_attrs():
    da = make_dense_dataset(ndim=3, attrs=True)["Sample"]
    pa = PlotArray(data=da.data, meta=da.meta)
    assert_identical(pa['x', 10], da['x', 10])


def test_plot_array_view_index_with_binedges():
    da = make_dense_dataset(ndim=3, binedges=True)["Sample"]
    pa = PlotArray(data=da.data, meta=da.coords)
    assert_identical(pa['x', 10], da['x', 10])


def test_plot_array_view_range_with_binedges():
    da = make_dense_dataset(ndim=3, binedges=True)["Sample"]
    pa = PlotArray(data=da.data, meta=da.coords)
    assert_identical(pa['y', 5:10], da['y', 5:10])


def test_plot_array_view_ragged():
    da = make_dense_dataset(ndim=2, ragged=True)["Sample"]
    pa = PlotArray(data=da.data, meta=da.coords)
    assert_identical(pa['tof', 5:10], da['tof', 5:10])


def test_plot_array_view_ragged_with_masks():
    da = make_dense_dataset(ndim=2, ragged=True, masks=True)["Sample"]
    pa = PlotArray(data=da.data, meta=da.coords, masks=da.masks)
    assert_identical(pa['tof', 5:10], da['tof', 5:10])


def test_plot_array_view_ragged_with_masks_with_binedges():
    da = make_dense_dataset(ndim=2, ragged=True, masks=True,
                            binedges=True)["Sample"]
    pa = PlotArray(data=da.data, meta=da.coords, masks=da.masks)
    assert_identical(pa['tof', 5:10], da['tof', 5:10])


def test_plot_array_view_of_view_index():
    da = make_dense_dataset(ndim=3)["Sample"]
    pa = PlotArray(data=da.data, meta=da.coords)
    assert_identical(pa['x', 10]['y', 10], da['x', 10]['y', 10])


def test_plot_array_view_of_view_range():
    da = make_dense_dataset(ndim=3)["Sample"]
    pa = PlotArray(data=da.data, meta=da.coords)
    assert_identical(pa['x', 5:10]['y', 5:10], da['x', 5:10]['y', 5:10])
