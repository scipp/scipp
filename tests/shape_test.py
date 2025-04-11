# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file

import numpy as np
import pytest

import scipp as sc


def test_broadcast_variable() -> None:
    x = sc.arange('x', 3)
    assert sc.identical(
        sc.broadcast(x, sizes={'x': 3, 'y': 2}),
        sc.array(dims=['x', 'y'], values=[[0, 0], [1, 1], [2, 2]]),
    )
    assert sc.identical(
        sc.broadcast(x, dims=['x', 'y'], shape=[3, 2]),
        sc.array(dims=['x', 'y'], values=[[0, 0], [1, 1], [2, 2]]),
    )


def test_broadcast_data_array() -> None:
    N = 6
    d = sc.linspace('x', 2.0, 10.0, N)
    x = sc.arange('x', float(N))
    m = x < 3.0
    da = sc.DataArray(d, coords={'x': x}, masks={'m': m})
    expected = sc.DataArray(
        sc.broadcast(d, sizes={'x': 6, 'y': 3}),
        coords={'x': x},
        masks={'m': m},
    )
    assert sc.identical(sc.broadcast(da, sizes={'x': 6, 'y': 3}), expected)
    assert sc.identical(sc.broadcast(da, dims=['x', 'y'], shape=[6, 3]), expected)


def test_broadcast_fails_with_bad_inputs() -> None:
    x = sc.array(dims=['x'], values=np.arange(6.0))
    with pytest.raises(ValueError, match='dims and shape must both be None'):
        _ = sc.broadcast(x, sizes={'x': 6, 'y': 3}, dims=['x', 'y'], shape=[6, 3])  # type: ignore[call-overload]
    with pytest.raises(ValueError, match='dims and shape must both be None'):
        _ = sc.broadcast(x, sizes={'x': 6, 'y': 3}, dims=['x', 'y'])  # type: ignore[call-overload]
    with pytest.raises(ValueError, match='dims and shape must both be None'):
        _ = sc.broadcast(x, sizes={'x': 6, 'y': 3}, shape=[6, 3])  # type: ignore[call-overload]


def test_concat() -> None:
    var = sc.scalar(1.0)
    assert sc.identical(
        sc.concat([var, var + var, 3 * var], 'x'),
        sc.array(dims=['x'], values=[1.0, 2.0, 3.0]),
    )


def test_concat_data_group() -> None:
    var = sc.scalar(1.0)
    dg = sc.DataGroup({'a': var})
    result = sc.concat([dg, dg + dg], 'x')
    assert isinstance(result, sc.DataGroup)
    assert sc.identical(result['a'], sc.array(dims=['x'], values=[1.0, 2.0]))


def test_fold_variable() -> None:
    var = sc.arange('f', 6)
    assert sc.identical(
        sc.fold(var, dim='f', sizes={'g': 2, 'h': 3}),
        sc.array(dims=['g', 'h'], values=[[0, 1, 2], [3, 4, 5]]),
    )
    assert sc.identical(
        sc.fold(var, dim='f', dims=['g', 'h'], shape=[2, 3]),
        sc.array(dims=['g', 'h'], values=[[0, 1, 2], [3, 4, 5]]),
    )


def test_fold_data_array() -> None:
    da = sc.DataArray(sc.arange('f', 6))
    assert sc.identical(
        sc.fold(da, dim='f', sizes={'g': 2, 'h': 3}),
        sc.DataArray(sc.array(dims=['g', 'h'], values=[[0, 1, 2], [3, 4, 5]])),
    )
    assert sc.identical(
        sc.fold(da, dim='f', dims=['g', 'h'], shape=[2, 3]),
        sc.DataArray(sc.array(dims=['g', 'h'], values=[[0, 1, 2], [3, 4, 5]])),
    )


def test_fold_size_minus_1_variable() -> None:
    x = sc.array(dims=['x'], values=np.arange(6.0))
    assert sc.identical(
        sc.fold(x, dim='x', sizes={'x': 2, 'y': 3}),
        sc.fold(x, dim='x', sizes={'x': 2, 'y': -1}),
    )
    assert sc.identical(
        sc.fold(x, dim='x', sizes={'x': 2, 'y': 3}),
        sc.fold(x, dim='x', sizes={'x': -1, 'y': 3}),
    )


def test_fold_size_minus_1_data_array() -> None:
    x = sc.array(dims=['x'], values=np.arange(6.0))
    da = sc.DataArray(x)
    assert sc.identical(
        sc.fold(da, dim='x', sizes={'x': 2, 'y': 3}),
        sc.fold(da, dim='x', sizes={'x': 2, 'y': -1}),
    )
    assert sc.identical(
        sc.fold(da, dim='x', sizes={'x': 2, 'y': 3}),
        sc.fold(da, dim='x', sizes={'x': -1, 'y': 3}),
    )


def test_fold_raises_two_minus_1() -> None:
    x = sc.array(dims=['x'], values=np.arange(6.0))
    da = sc.DataArray(x)
    with pytest.raises(sc.DimensionError):
        sc.fold(x, dim='x', sizes={'x': -1, 'y': -1})
    with pytest.raises(sc.DimensionError):
        sc.fold(da, dim='x', sizes={'x': -1, 'y': -1})


def test_fold_raises_non_divisible() -> None:
    x = sc.array(dims=['x'], values=np.arange(10.0))
    da = sc.DataArray(x)
    with pytest.raises(ValueError, match=r'original shape \d+ cannot be divided by'):
        sc.fold(x, dim='x', sizes={'x': 3, 'y': -1})
    with pytest.raises(ValueError, match=r'original shape \d+ cannot be divided by'):
        sc.fold(da, dim='x', sizes={'x': -1, 'y': 3})


def test_flatten_variable() -> None:
    var = sc.arange('r', 6).fold('r', sizes={'a': 2, 'b': 3})
    assert sc.identical(sc.flatten(var, dims=['a', 'b'], to='f'), sc.arange('f', 6))
    assert sc.identical(sc.flatten(var, to='f'), sc.arange('f', 6))


def test_flatten_data_array() -> None:
    da = sc.DataArray(sc.arange('r', 6).fold('r', sizes={'a': 2, 'b': 3}))
    assert sc.identical(
        sc.flatten(da, dims=['a', 'b'], to='f'), sc.DataArray(sc.arange('f', 6))
    )
    assert sc.identical(sc.flatten(da, to='f'), sc.DataArray(sc.arange('f', 6)))


def test_squeeze() -> None:
    xy = sc.arange('a', 2).fold('a', sizes={'x': 1, 'y': 2})
    assert sc.identical(sc.squeeze(xy, dim='x'), sc.arange('y', 2))
    assert sc.identical(sc.squeeze(xy, dim=['x']), sc.arange('y', 2))
    assert sc.identical(sc.squeeze(xy), sc.arange('y', 2))
