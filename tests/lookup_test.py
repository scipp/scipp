# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import numpy as np
import scipp as sc


@pytest.mark.parametrize('mode', ['nearest', 'previous'])
def test_raises_with_histogram_if_mode_set(mode):
    da = sc.DataArray(sc.arange('x', 4), coords={'x': sc.arange('x', 5)})
    with pytest.raises(ValueError):
        sc.lookup(da, mode=mode)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_histogram(dtype):
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
def test_values_in_masked_bins_replaced_by_fill_value(dtype):
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
def test_previous(dtype):
    x = sc.linspace(dim='xx', start=0, stop=1, num=4)
    data = sc.array(dims=['xx'], values=[0, 1, 0, 2], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.4, 0.1, 0.6, 0.9, 1.1, 0.2])
    expected = sc.array(dims=['event'], values=[0, 1, 0, 1, 0, 2, 0], dtype=dtype)
    assert sc.identical(sc.lookup(da, mode='previous')(var), expected)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_nearest(dtype):
    x = sc.linspace(dim='xx', start=0, stop=1, num=5)
    data = sc.array(dims=['xx'], values=[0, 1, 0, 2, 2], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.4, 0.1, 0.6, 0.9, 1.1, 0.2])
    expected = sc.array(dims=['event'], values=[0, 0, 0, 0, 2, 2, 1], dtype=dtype)
    assert sc.identical(sc.lookup(da, mode='nearest')(var), expected)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_previous_masked_points_replaced_by_fill_value(dtype):
    x = sc.linspace(dim='xx', start=0, stop=1, num=4)
    data = sc.array(dims=['xx'], values=[0, 1, 0, 2], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    da.masks['mask'] = sc.array(dims=['xx'], values=[False, False, True, False])
    var = sc.array(dims=['event'], values=[0.1, 0.4, 0.1, 0.6, 0.9, 1.1, 0.2])
    expected = sc.array(dims=['event'], values=[0, 1, 0, 1, 666, 2, 0], dtype=dtype)
    fill = sc.scalar(666, dtype=dtype)
    assert sc.identical(sc.lookup(da, mode='previous', fill_value=fill)(var), expected)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_nearest_masked_points_replaced_by_fill_value(dtype):
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
        return np.NaN
    return 0


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
@pytest.mark.parametrize("mode", ['nearest', 'previous'])
def test_function_with_no_value_gives_fill_value(mode, dtype):
    x = sc.array(dims=['xx'], values=[])
    data = sc.array(dims=['xx'], values=[], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.5, 0.6])
    bad = outofbounds(dtype)
    expected = sc.array(dims=['event'], values=[bad, bad, bad], dtype=dtype)
    assert sc.identical(sc.lookup(da, mode=mode)(var), expected, equal_nan=True)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_previous_single_value(dtype):
    x = sc.array(dims=['xx'], values=[0.5])
    data = sc.array(dims=['xx'], values=[11], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.5, 0.6])
    expected = sc.array(dims=['event'],
                        values=[outofbounds(dtype), 11, 11],
                        dtype=dtype)
    assert sc.identical(sc.lookup(da, mode='previous')(var), expected, equal_nan=True)


@pytest.mark.parametrize("dtype", ['bool', 'int32', 'int64', 'float32', 'float64'])
def test_nearest_single_value(dtype):
    x = sc.array(dims=['xx'], values=[0.5])
    data = sc.array(dims=['xx'], values=[11], dtype=dtype)
    da = sc.DataArray(data=data, coords={'xx': x})
    var = sc.array(dims=['event'], values=[0.1, 0.5, 0.6])
    expected = sc.array(dims=['event'], values=[11, 11, 11], dtype=dtype)
    assert sc.identical(sc.lookup(da, mode='nearest')(var), expected)
