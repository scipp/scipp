# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file

import numpy as np
import pytest
import scipp as sc
from .common import assert_export


def test_concat():
    var = sc.scalar(1.0)
    assert sc.identical(sc.concat([var, var + var, 3 * var], 'x'),
                        sc.array(dims=['x'], values=[1.0, 2.0, 3.0]))


def test_fold():
    x = sc.array(dims=['x'], values=np.arange(6.0))
    da = sc.DataArray(x)
    assert_export(sc.fold, x=x, sizes={'x': 2, 'y': 3})
    assert_export(sc.fold, x=x, dims=['x', 'y'], shape=[2, 3])
    assert_export(sc.fold, x=da, sizes={'x': 2, 'y': 3})
    assert_export(sc.fold, x=da, dims=['x', 'y'], shape=[2, 3])


def test_fold_size_minus_1():
    x = sc.array(dims=['x'], values=np.arange(6.0))
    da = sc.DataArray(x)
    assert sc.identical(sc.fold(x, dim='x', sizes={
        'x': 2,
        'y': 3
    }), sc.fold(x, dim='x', sizes={
        'x': 2,
        'y': -1
    }))
    assert sc.identical(sc.fold(x, dim='x', sizes={
        'x': 2,
        'y': 3
    }), sc.fold(x, dim='x', sizes={
        'x': -1,
        'y': 3
    }))
    assert sc.identical(sc.fold(da, dim='x', sizes={
        'x': 2,
        'y': 3
    }), sc.fold(da, dim='x', sizes={
        'x': 2,
        'y': -1
    }))
    assert sc.identical(sc.fold(da, dim='x', sizes={
        'x': 2,
        'y': 3
    }), sc.fold(da, dim='x', sizes={
        'x': -1,
        'y': 3
    }))
    with pytest.raises(sc.DimensionError):
        sc.fold(x, dim='x', sizes={'x': -1, 'y': -1})
        sc.fold(da, dim='x', sizes={'x': -1, 'y': -1})


def test_flatten():
    x = sc.array(dims=['x', 'y'], values=np.arange(6.0).reshape(2, 3))
    da = sc.DataArray(x)
    assert_export(sc.flatten, x=x, dims=['x', 'y'], dim='z')
    assert_export(sc.flatten, x=x, dim='z')
    assert_export(sc.flatten, x=da, dims=['x', 'y'], dim='z')
    assert_export(sc.flatten, x=da, dim='z')


def test_squeeze():
    xy = sc.arange('a', 2).fold('a', {'x': 1, 'y': 2})
    assert sc.identical(sc.squeeze(xy, dim='x'), sc.arange('y', 2))
    assert sc.identical(sc.squeeze(xy, dim=['x']), sc.arange('y', 2))
    assert sc.identical(sc.squeeze(xy), sc.arange('y', 2))
