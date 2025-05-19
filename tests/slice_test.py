# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def test_setitem_ellipsis_variable() -> None:
    var = sc.ones(dims=['x'], shape=[2])
    ref = var
    ref[...] = 1.2 * sc.units.one
    assert sc.identical(var, sc.array(dims=['x'], values=[1.2, 1.2]))


def test_setitem_ellipsis_data_array() -> None:
    var = sc.ones(dims=['x'], shape=[2])
    da = sc.DataArray(data=var)
    expected = da + 0.2 * sc.units.one
    da[...] = 1.2 * sc.units.one
    assert sc.identical(da, expected)
    assert sc.identical(var, da.data)
    da.data[...] = 2.3 * sc.units.one
    assert sc.identical(var, da.data)


def test_setitem_ellipsis_dataset() -> None:
    var = sc.ones(dims=['x'], shape=[2])
    da = sc.DataArray(data=var)
    ds = sc.Dataset(data={'a': da})
    expected = ds + 0.2 * sc.units.one
    ds[...] = 1.2 * sc.units.one
    assert sc.identical(ds, expected)
    assert sc.identical(var, ds['a'].data)
    ds['a'][...] = 2.3 * sc.units.one
    assert sc.identical(var, ds['a'].data)
    ds['a'].data[...] = 2.3 * sc.units.one
    assert sc.identical(var, ds['a'].data)


def test_ipython_key_completion() -> None:
    var = sc.ones(dims=['x', 'y'], shape=[2, 1])
    da = sc.DataArray(data=var, coords={'x': var, 'aux': var})
    assert set(var._ipython_key_completions_()) == set(var.dims)
    assert set(da._ipython_key_completions_()) == set(da.dims)


xx = sc.arange(dim='xx', start=2, stop=6)
ds = sc.Dataset(data={'a': xx, 'b': xx + 1})


@pytest.mark.parametrize("obj", [xx.copy(), ds['a'].copy(), ds.copy()])
def test_slice_implicit_dim(obj: sc.Variable | sc.DataArray | sc.Dataset) -> None:
    assert sc.identical(obj[1], obj['xx', 1])
    assert sc.identical(obj[1:3], obj['xx', 1:3])
    obj[1] = obj[0]
    assert sc.identical(obj[1], obj['xx', 0])
    obj[1:3] = obj[0]
    assert sc.identical(obj[2], obj['xx', 0])


def test_getitem_with_stride_equivalent_to_numpy() -> None:
    var = sc.arange('x', 10)
    assert np.array_equal(var['x', 2:7:2].values, var.values[2:7:2])
    assert np.array_equal(var['x', 7:7:2].values, var.values[7:7:2])
    assert np.array_equal(var['x', 2:7:3].values, var.values[2:7:3])
    assert np.array_equal(var['x', 3:7:3].values, var.values[3:7:3])
    assert np.array_equal(var['x', 4:7:3].values, var.values[4:7:3])
    assert np.array_equal(var['x', 5:7:3].values, var.values[5:7:3])
    assert np.array_equal(var['x', 2:7:99].values, var.values[2:7:99])
    assert np.array_equal(var['x', :7:2].values, var.values[:7:2])
    assert np.array_equal(var['x', 2::2].values, var.values[2::2])
    assert np.array_equal(var['x', ::2].values, var.values[::2])
    assert np.array_equal(var['x', -4::2].values, var.values[-4::2])
    assert np.array_equal(var['x', :-4:2].values, var.values[:-4:2])


def test_setitem_with_stride_2_sets_every_other_element() -> None:
    var = sc.array(dims=['x'], values=[1, 2, 3, 4, 5, 6])
    var['x', 1:5:2] = sc.array(dims=['x'], values=[11, 22])
    assert sc.identical(var, sc.array(dims=['x'], values=[1, 11, 3, 22, 5, 6]))


def test_setitem_data_array_value_based_slice() -> None:
    da = sc.data.table_xyz(10)
    var = sc.scalar(44.0, unit='K')
    da['x', da.coords['x'][0]] = var
    assert sc.identical(da[0].data, var)
    da['x', da.coords['x'][0]] = da['x', da.coords['x'][1]]
    assert sc.identical(da[0].data, da[1].data)


def test_setitem_dataset_value_based_slice() -> None:
    ds = sc.Dataset({'a': sc.data.table_xyz(10), 'b': sc.data.table_xyz(10) * 1.123})
    ds['x', ds.coords['x'][0]] = ds['x', ds.coords['x'][1]]
    assert sc.identical(ds['a'][0].data, ds['a'][1].data)
    assert sc.identical(ds['b'][0].data, ds['b'][1].data)


def test_dataset_slice_range_then_get_item() -> None:
    a = sc.arange('xx', 5)
    ds = sc.Dataset({'a': a}, coords={'xx': sc.arange('xx', 0, 10, 2)})
    assert sc.identical(ds['xx', 1:-1]['a'].data, a['xx', 1:-1])
    assert sc.identical(ds['xx', 1:]['a'].data, a['xx', 1:])
    assert sc.identical(ds['xx', :-1]['a'].data, a['xx', :-1])
    assert sc.identical(ds['xx', :]['a'].data, a['xx', :])


def test_dataset_slice_single_index_then_get_item() -> None:
    a = sc.arange('xx', 5)
    ds = sc.Dataset({'a': a}, coords={'xx': sc.arange('xx', 0, 10, 2)})
    assert sc.identical(ds['xx', -2]['a'].data, a['xx', -2])


def test_dataset_set_slice_range() -> None:
    ds = sc.Dataset(
        {'a': sc.arange('xx', 5), 'b': sc.arange('xx', 5, 10)},
        coords={'xx': sc.arange('xx', 0, 10, 2)},
    )
    aa = sc.array(dims=['xx'], values=[-7, -4])
    ds['xx', 2:4] = aa
    assert sc.identical(ds['a'].data, sc.array(dims=['xx'], values=[0, 1, -7, -4, 4]))
    assert sc.identical(ds['b'].data, sc.array(dims=['xx'], values=[5, 6, -7, -4, 9]))
