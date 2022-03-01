# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
import numpy as np

import scipp as sc


def test_iadd_dataset_with_dataarray():
    da = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(1.0, 10.0)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(1.0, 10.0))})
    ds = sc.Dataset({'data': da.copy()})
    expected = sc.Dataset({'data': da + da})
    ds += da
    assert sc.identical(ds, expected)


def test_isub_dataset_with_dataarray():
    da = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(1.0, 10.0)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(1.0, 10.0))})
    ds = sc.Dataset({'data': da.copy()})
    expected = sc.Dataset({'data': da - da})
    ds -= da
    assert sc.identical(ds, expected)


def test_imul_dataset_with_dataarray():
    da = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(1.0, 10.0)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(1.0, 10.0))})
    ds = sc.Dataset({'data': da.copy()})
    expected = sc.Dataset({'data': da * da})
    ds *= da
    assert sc.identical(ds, expected)


def test_itruediv_dataset_with_dataarray():
    da = sc.DataArray(
        data=sc.Variable(dims=['x'], values=np.arange(1.0, 10.0)),
        coords={'x': sc.Variable(dims=['x'], values=np.arange(1.0, 10.0))})
    ds = sc.Dataset({'data': da.copy()})
    expected = sc.Dataset({'data': da / da})
    ds /= da
    assert sc.identical(ds, expected)


def test_iadd_dataset_with_scalar():
    ds = sc.Dataset(data={'data': sc.Variable(dims=['x'], values=np.arange(10.0))},
                    coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})
    expected = ds.copy()
    expected['data'] = ds['data'] + 2.0

    ds += 2.0
    assert sc.identical(ds, expected)


def test_isub_dataset_with_scalar():
    ds = sc.Dataset(data={'data': sc.Variable(dims=['x'], values=np.arange(10.0))},
                    coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})
    expected = ds.copy()
    expected['data'] = ds['data'] - 3.0

    ds -= 3.0
    assert sc.identical(ds, expected)


def test_imul_dataset_with_scalar():
    ds = sc.Dataset(data={'data': sc.Variable(dims=['x'], values=np.arange(10.0))},
                    coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})
    expected = ds.copy()
    expected['data'] = ds['data'] * 1.5

    ds *= 1.5
    assert sc.identical(ds, expected)


def test_itruediv_dataset_with_scalar():
    ds = sc.Dataset(data={'data': sc.Variable(dims=['x'], values=np.arange(10.0))},
                    coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})
    expected = ds.copy()
    expected['data'] = ds['data'] / 0.5

    ds /= 0.5
    assert sc.identical(ds, expected)


def test_isub_dataset_with_dataset_broadcast():
    ds = sc.Dataset(data={'data': sc.Variable(dims=['x'], values=np.arange(10.0))},
                    coords={'x': sc.Variable(dims=['x'], values=np.arange(10.0))})

    expected = ds - ds['x', 0]
    ds -= ds['x', 0]
    assert sc.identical(ds, expected)
