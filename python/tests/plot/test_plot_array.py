# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file

import scipp as sc
from scipp.plot.helpers import PlotArray
from plot_helper import make_dense_dataset


def assert_is_equal(plotarray, dataarray):
    assert sc.is_equal(plotarray.data, dataarray.data)
    for key in dataarray.coords:
        assert sc.is_equal(plotarray.coords[str(key)], dataarray.coords[key])
    for msk in dataarray.masks:
        assert sc.is_equal(plotarray.masks[msk], dataarray.masks[msk])


def test_plot_array_creation():
    da = make_dense_dataset(ndim=3, attrs=True, masks=True)["Sample"]
    pa = PlotArray(data=da.data,
                   coords=da.coords,
                   attrs=da.attrs,
                   masks=da.masks)
    assert_is_equal(pa, da)


def test_plot_array_view_index():
    da = make_dense_dataset(ndim=3)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords)
    assert_is_equal(pa['x', 10], da['x', 10])


def test_plot_array_view_range():
    da = make_dense_dataset(ndim=3)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords)
    assert_is_equal(pa['y', 5:10], da['y', 5:10])


def test_plot_array_view_index_with_masks():
    da = make_dense_dataset(ndim=3, masks=True)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords, masks=da.masks)
    assert_is_equal(pa['x', 10], da['x', 10])


def test_plot_array_view_index_with_attrs():
    da = make_dense_dataset(ndim=3, attrs=True)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords, attrs=da.attrs)
    assert_is_equal(pa['x', 10], da['x', 10])


def test_plot_array_view_index_with_binedges():
    da = make_dense_dataset(ndim=3, binedges=True)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords)
    assert_is_equal(pa['x', 10], da['x', 10])


def test_plot_array_view_range_with_binedges():
    da = make_dense_dataset(ndim=3, binedges=True)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords)
    assert_is_equal(pa['y', 5:10], da['y', 5:10])


def test_plot_array_view_ragged():
    da = make_dense_dataset(ndim=2, ragged=True)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords)
    assert_is_equal(pa['tof', 5:10], da['tof', 5:10])


def test_plot_array_view_ragged_with_masks():
    da = make_dense_dataset(ndim=2, ragged=True, masks=True)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords, masks=da.masks)
    assert_is_equal(pa['tof', 5:10], da['tof', 5:10])


def test_plot_array_view_ragged_with_masks_with_binedges():
    da = make_dense_dataset(ndim=2, ragged=True, masks=True,
                            binedges=True)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords, masks=da.masks)
    assert_is_equal(pa['tof', 5:10], da['tof', 5:10])


def test_plot_array_view_of_view_index():
    da = make_dense_dataset(ndim=3)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords)
    assert_is_equal(pa['x', 10]['y', 10], da['x', 10]['y', 10])


def test_plot_array_view_of_view_range():
    da = make_dense_dataset(ndim=3)["Sample"]
    pa = PlotArray(data=da.data, coords=da.coords)
    assert_is_equal(pa['x', 5:10]['y', 5:10], da['x', 5:10]['y', 5:10])
