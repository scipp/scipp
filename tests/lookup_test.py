# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from operator import mul, truediv

import numpy as np
import pytest

import scipp as sc


@pytest.mark.parametrize('mode', ['nearest', 'previous'])
def test_raises_with_histogram_if_mode_set(mode) -> None:
    da = sc.DataArray(sc.arange('x', 4), coords={'x': sc.arange('x', 5)})
    with pytest.raises(ValueError, match='Input is a histogram'):
        sc.lookup(da, mode=mode)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_histogram(dtype) -> None:
    x_lin = sc.linspace(dim='xx', start=0, stop=1, num=4)
    x = x_lin.copy()
    x.values[0] -= 0.01
    data = sc.array(dims=['xx'], values=[0, 1, 0], dtype=dtype)
    hist_lin = sc.DataArray(data=data, coords={'xx': x_lin})
    hist = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.4, 0.1, 0.6, 0.9, 0.2])
    lut = sc.lookup(hist, 'xx')
    expected = sc.array(dims=['event'], values=[0, 1, 0, 1, 0, 0], dtype=dtype)
    assert sc.identical(lut(var), expected)
    lut = sc.lookup(hist_lin, 'xx')
    assert sc.identical(lut(var), expected)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_values_in_masked_bins_replaced_by_fill_value(dtype) -> None:
    x_lin = sc.linspace(dim='xx', start=0, stop=1, num=4)
    x = x_lin.copy()
    x.values[0] -= 0.01
    data = sc.array(dims=['xx'], values=[0, 1, 0], dtype=dtype)
    hist_lin = sc.DataArray(data=data, coords={'xx': x_lin})
    hist = sc.DataArray(data=data, coords={'xx': x})
    mask = sc.array(dims=['xx'], values=[False, False, True])
    hist_lin.masks['mask'] = mask
    hist.masks['mask'] = mask
    var = sc.array(dims=['event'], values=[0.1, 0.4, 0.1, 0.6, 0.9, 0.2])
    fill = sc.scalar(666, dtype=dtype)
    lut = sc.lookup(hist, fill_value=fill)
    expected = sc.array(dims=['event'], values=[0, 1, 0, 1, 666, 0], dtype=dtype)
    assert sc.identical(lut(var), expected)
    lut = sc.lookup(hist_lin, fill_value=fill)
    assert sc.identical(lut(var), expected)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_previous(dtype) -> None:
    x = sc.linspace(dim='xx', start=0, stop=1, num=4)
    data = sc.array(dims=['xx'], values=[0, 1, 0, 2], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.4, 0.1, 0.6, 0.9, 1.1, 0.2, -0.1])
    expected = sc.array(dims=['event'], values=[0, 1, 0, 1, 0, 2, 0, 666], dtype=dtype)
    fill = sc.scalar(666, dtype=dtype)
    assert sc.identical(sc.lookup(da, mode='previous', fill_value=fill)(var), expected)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_nearest(dtype) -> None:
    x = sc.linspace(dim='xx', start=0, stop=1, num=5)
    data = sc.array(dims=['xx'], values=[0, 1, 0, 2, 2], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.4, 0.1, 0.6, 0.9, 1.1, 0.2, -0.1])
    expected = sc.array(dims=['event'], values=[0, 0, 0, 0, 2, 2, 1, 0], dtype=dtype)
    fill = sc.scalar(666, dtype=dtype)
    assert sc.identical(sc.lookup(da, mode='nearest', fill_value=fill)(var), expected)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_previous_masked_points_replaced_by_fill_value(dtype) -> None:
    x = sc.linspace(dim='xx', start=0, stop=1, num=4)
    data = sc.array(dims=['xx'], values=[0, 1, 0, 2], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    da.masks['mask'] = sc.array(dims=['xx'], values=[False, False, True, False])
    var = sc.array(dims=['event'], values=[0.1, 0.4, 0.1, 0.6, 0.9, 1.1, 0.2])
    expected = sc.array(dims=['event'], values=[0, 1, 0, 1, 666, 2, 0], dtype=dtype)
    fill = sc.scalar(666, dtype=dtype)
    assert sc.identical(sc.lookup(da, mode='previous', fill_value=fill)(var), expected)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_nearest_masked_points_replaced_by_fill_value(dtype) -> None:
    x = sc.linspace(dim='xx', start=0, stop=1, num=5)
    data = sc.array(dims=['xx'], values=[0, 1, 0, 2, 2], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    da.masks['mask'] = sc.array(dims=['xx'], values=[False, False, True, False, False])
    var = sc.array(dims=['event'], values=[0.1, 0.4, 0.1, 0.6, 0.9, 1.1, 0.2])
    expected = sc.array(dims=['event'], values=[0, 666, 0, 666, 2, 2, 1], dtype=dtype)
    fill = sc.scalar(666, dtype=dtype)
    assert sc.identical(sc.lookup(da, mode='nearest', fill_value=fill)(var), expected)


def outofbounds(dtype):
    if dtype in ['float32', 'float64']:
        return np.nan
    return 0


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
@pytest.mark.parametrize("mode", ['nearest', 'previous'])
def test_function_with_no_value_gives_fill_value(mode, dtype) -> None:
    x = sc.array(dims=['xx'], values=[])
    data = sc.array(dims=['xx'], values=[], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.5, 0.6])
    bad = outofbounds(dtype)
    expected = sc.array(dims=['event'], values=[bad, bad, bad], dtype=dtype)
    assert sc.identical(sc.lookup(da, mode=mode)(var), expected, equal_nan=True)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_previous_single_value(dtype) -> None:
    x = sc.array(dims=['xx'], values=[0.5])
    data = sc.array(dims=['xx'], values=[11], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.5, 0.6])
    expected = sc.array(
        dims=['event'], values=[outofbounds(dtype), 11, 11], dtype=dtype
    )
    assert sc.identical(sc.lookup(da, mode='previous')(var), expected, equal_nan=True)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_nearest_single_value(dtype) -> None:
    x = sc.array(dims=['xx'], values=[0.5])
    data = sc.array(dims=['xx'], values=[11], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.5, 0.6])
    expected = sc.array(dims=['event'], values=[11, 11, 11], dtype=dtype)
    assert sc.identical(sc.lookup(da, mode='nearest')(var), expected)


def test_ignores_unrelated_coords() -> None:
    var = sc.Variable(dims=['event'], values=[1.0, 2.0, 3.0, 4.0])
    table = sc.DataArray(var, coords={'x': var})
    binned = table.bin(x=sc.array(dims=['x'], values=[1.0, 5.0]))
    hist = sc.DataArray(
        data=sc.Variable(dims=['x'], values=[1.0, 2.0]),
        coords={'x': sc.Variable(dims=['x'], values=[1.0, 3.0, 5.0])},
    )
    hist.coords['scalar'] = sc.scalar(1.2)
    result = binned.bins * sc.lookup(func=hist, dim='x')
    assert 'scalar' not in result.coords
    assert 'scalar' not in result.bins.coords


@pytest.mark.parametrize("dtype", ['float32', 'float64'])
@pytest.mark.parametrize("op", [mul, truediv])
def test_promotes_to_dtype_of_lut(op, dtype) -> None:
    da = sc.data.table_xyz(10).to(dtype='float32').bin(x=2)
    edges = sc.array(dims=['x'], unit='m', values=[0.0, 0.5, 1.0])
    weight = sc.array(dims=['x'], values=[10.0, 3.0], dtype=dtype)
    hist = sc.DataArray(weight, coords={'x': edges})
    result = op(da.bins, sc.lookup(func=hist, dim='x'))
    assert result.bins.constituents['data'].dtype == dtype


@pytest.mark.parametrize("dtype", ['float32', 'float64'])
@pytest.mark.parametrize("op", [mul, truediv])
def test_promotes_to_dtype_of_events(op, dtype) -> None:
    da = sc.data.table_xyz(10).to(dtype=dtype).bin(x=2)
    edges = sc.array(dims=['x'], unit='m', values=[0.0, 0.5, 1.0])
    weight = sc.array(dims=['x'], values=[10.0, 3.0], dtype='float32')
    hist = sc.DataArray(weight, coords={'x': edges})
    result = op(da.bins, sc.lookup(func=hist, dim='x'))
    assert result.bins.constituents['data'].dtype == dtype


def test_lookup_2d_coord() -> None:
    edges = sc.array(dims=['x', 'y'], values=[[0.0, 1.0, 2.0], [1.0, 2.0, 3.0]])
    da = sc.DataArray(
        sc.array(dims=['x', 'y'], values=[[10.0, 11.0], [12.0, 13.0]]),
        coords={'y': edges},
    )

    expected = sc.array(
        dims=['x', 'y'], values=[[10.0, 11.0, np.nan], [12.0, 13.0, np.nan]]
    )

    assert sc.identical(sc.lookup(da, dim='y')[edges + 0.1], expected, equal_nan=True)


def test_lookup_2d_coord_with_mask() -> None:
    edges = sc.array(dims=['x', 'y'], values=[[0.0, 1.0, 2.0], [1.0, 2.0, 3.0]])
    da = sc.DataArray(
        sc.array(dims=['x', 'y'], values=[[10.0, 11.0], [12.0, 13.0]]),
        coords={'y': edges},
        masks={'m': sc.array(dims=['x', 'y'], values=[[False, False], [True, False]])},
    )
    fill = sc.scalar(99.0)

    expected = sc.array(
        dims=['x', 'y'],
        values=[[10.0, 11.0, fill.value], [fill.value, 13.0, fill.value]],
    )

    assert sc.identical(
        sc.lookup(da, dim='y', fill_value=fill)[edges + 0.1], expected, equal_nan=True
    )


def test_lookup_2d_coord_with_1d_mask() -> None:
    edges = sc.array(dims=['x', 'y'], values=[[0.0, 1.0, 2.0], [1.0, 2.0, 3.0]])
    da = sc.DataArray(
        sc.array(dims=['x', 'y'], values=[[10.0, 11.0], [12.0, 13.0]]),
        coords={'y': edges},
        masks={'m': sc.array(dims=['y'], values=[True, False])},
    )
    fill = sc.scalar(99.0)

    expected = sc.array(
        dims=['x', 'y'],
        values=[[fill.value, 11.0, fill.value], [fill.value, 13.0, fill.value]],
    )

    assert sc.identical(
        sc.lookup(da, dim='y', fill_value=fill)[edges + 0.1], expected, equal_nan=True
    )


def test_lookup_2d_coord_outer_dim() -> None:
    edges = (
        sc.array(dims=['x', 'y'], values=[[0.0, 1.0, 2.0], [1.0, 2.0, 3.0]])
        .transpose()
        .copy()
    )
    da = sc.DataArray(
        sc.array(dims=['x', 'y'], values=[[10.0, 11.0], [12.0, 13.0]])
        .transpose()
        .copy(),
        coords={'y': edges},
    )

    expected = (
        sc.array(dims=['x', 'y'], values=[[10.0, 11.0, np.nan], [12.0, 13.0, np.nan]])
        .transpose()
        .copy()
    )

    assert sc.identical(sc.lookup(da, dim='y')[edges + 0.1], expected, equal_nan=True)


def test_lookup_2d_coord_outer_dim_with_mask() -> None:
    edges = (
        sc.array(dims=['x', 'y'], values=[[0.0, 1.0, 2.0], [1.0, 2.0, 3.0]])
        .transpose()
        .copy()
    )
    da = sc.DataArray(
        sc.array(dims=['x', 'y'], values=[[10.0, 11.0], [12.0, 13.0]])
        .transpose()
        .copy(),
        coords={'y': edges},
        masks={
            'm': sc.array(dims=['x', 'y'], values=[[False, False], [True, False]])
            .transpose()
            .copy()
        },
    )
    fill = sc.scalar(99.0)

    expected = (
        sc.array(
            dims=['x', 'y'],
            values=[[10.0, 11.0, fill.value], [fill.value, 13.0, fill.value]],
        )
        .transpose()
        .copy()
    )

    assert sc.identical(
        sc.lookup(da, dim='y', fill_value=fill)[edges + 0.1], expected, equal_nan=True
    )


def test_lookup_2d_coord_outer_dim_with_1d_mask() -> None:
    edges = (
        sc.array(dims=['x', 'y'], values=[[0.0, 1.0, 2.0], [1.0, 2.0, 3.0]])
        .transpose()
        .copy()
    )
    da = sc.DataArray(
        sc.array(dims=['x', 'y'], values=[[10.0, 11.0], [12.0, 13.0]])
        .transpose()
        .copy(),
        coords={'y': edges},
        masks={'m': sc.array(dims=['y'], values=[True, False])},
    )
    fill = sc.scalar(99.0)

    expected = (
        sc.array(
            dims=['x', 'y'],
            values=[[fill.value, 11.0, fill.value], [fill.value, 13.0, fill.value]],
        )
        .transpose()
        .copy()
    )

    assert sc.identical(
        sc.lookup(da, dim='y', fill_value=fill)[edges + 0.1], expected, equal_nan=True
    )
