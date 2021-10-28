# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc


def test_setitem_ellipsis_variable():
    var = sc.ones(dims=['x'], shape=[2])
    ref = var
    ref[...] = 1.2 * sc.units.one
    assert sc.identical(var, sc.array(dims=['x'], values=[1.2, 1.2]))


def test_setitem_ellipsis_data_array():
    var = sc.ones(dims=['x'], shape=[2])
    da = sc.DataArray(data=var)
    expected = da + 0.2 * sc.units.one
    da[...] = 1.2 * sc.units.one
    assert sc.identical(da, expected)
    assert sc.identical(var, da.data)
    da.data[...] = 2.3 * sc.units.one
    assert sc.identical(var, da.data)


def test_setitem_ellipsis_dataset():
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
