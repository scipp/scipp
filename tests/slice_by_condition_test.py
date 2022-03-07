# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest
import scipp as sc


def make_var() -> sc.Variable:
    return sc.arange('dummy', 12).fold(dim='dummy', sizes={'xx': 4, 'yy': 3})


def make_array() -> sc.DataArray:
    da = sc.DataArray(make_var())
    da.coords['xx'] = sc.arange('xx', 4)
    da.coords['yy'] = sc.arange('yy', 3)
    return da


def make_dataset() -> sc.Dataset:
    ds = sc.Dataset()
    ds['xy'] = make_array()
    ds['x'] = ds.coords['xx']
    return ds


@pytest.mark.parametrize("obj", [make_var(), make_array(), make_dataset()])
def test_all_false_gives_empty_slice(obj):
    condition = sc.array(dims=['xx'], values=[False, False, False, False])
    assert sc.identical(obj[condition], obj['xx', 0:0])


@pytest.mark.parametrize("obj", [make_var(), make_array(), make_dataset()])
def test_all_true_gives_copy(obj):
    condition = sc.array(dims=['xx'], values=[True, True, True, True])
    assert sc.identical(obj[condition], obj)


@pytest.mark.parametrize("obj", [make_var(), make_array(), make_dataset()])
def test_true_and_false_concats_slices(obj):
    condition = sc.array(dims=['xx'], values=[True, False, True, True])
    assert sc.identical(obj[condition], sc.concat([obj['xx', 0], obj['xx', 2:]], 'xx'))


def test_dataset_item_independent_of_condition_dim_preserved_unchanged():
    condition = sc.array(dims=['yy'], values=[True, False, True])
    ds = make_dataset()
    assert sc.identical(ds[condition]['x'], ds['x'])


def test_2d_condition_raises_DimensionError():
    var = make_var()
    with pytest.raises(sc.DimensionError):
        condition = var < 3
        var[condition]


def test_0d_condition_raises_DimensionError():
    var = make_var()
    with pytest.raises(sc.DimensionError):
        condition = sc.scalar(False)
        var[condition]
