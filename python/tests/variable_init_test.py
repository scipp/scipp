# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

import numpy as np
import pytest

import scipp as sc


@pytest.mark.parametrize(
    "value",
    [1.2, np.float64(1.2),
     sc.Variable(dims=(), values=1.2).value,
     np.array(1.2)])
def test_create_scalar_with_float_value(value):
    var = sc.Variable(dims=(), values=value)
    assert var.value == value
    assert var.dims == []
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.dimensionless


@pytest.mark.parametrize(
    "variance",
    [1.2, np.float64(1.2),
     sc.Variable(dims=(), values=1.2).value,
     np.array(1.2)])
def test_create_scalar_with_float_variance(variance):
    var = sc.Variable(dims=(), variances=variance)
    assert var.value == 0.0
    assert var.variance == variance
    assert var.dims == []
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.dimensionless


@pytest.mark.parametrize(
    "value",
    [1.2, np.float64(1.2),
     sc.Variable(dims=(), values=1.2).value,
     np.array(1.2)])
@pytest.mark.parametrize(
    "variance",
    [3.4, np.float64(3.4),
     sc.Variable(dims=(), values=3.4).value,
     np.array(3.4)])
def test_create_scalar_with_float_value_and_variance(value, variance):
    var = sc.Variable(dims=(), values=value, variances=variance)
    assert var.value == value
    assert var.variance == variance
    assert var.dims == []
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.dimensionless


@pytest.mark.parametrize('args', ((sc.dtype.int64, 1), (sc.dtype.bool, True),
                                  (sc.dtype.string, 'a')))
def test_create_scalar_with_value(args):
    dtype, value = args
    var = sc.Variable(dims=(), values=value)
    assert var.value == value
    assert var.dims == []
    assert var.dtype == dtype
    assert var.unit == sc.units.dimensionless


@pytest.mark.parametrize('args', ((sc.dtype.bool, True), (sc.dtype.string, 'a')))
def test_create_scalar_with_value_array(args):
    dtype, value = args
    var = sc.Variable(dims=(), values=np.array(value))
    assert var.value == value
    assert var.dims == []
    assert var.dtype == dtype
    assert var.unit == sc.units.dimensionless


def test_create_scalar_with_value_array_int():
    var = sc.Variable(dims=(), values=np.array(2))
    assert var.value == 2
    assert var.dims == []
    # The dtype varies between Windows and Linux / MacOS.
    assert var.dtype in (sc.dtype.int32, sc.dtype.int64)
    assert var.unit == sc.units.dimensionless


@pytest.mark.parametrize('variance', (1, True, 'a', sc.Variable(dims=(), values=1.2)))
def test_create_scalar_invalid_variance(variance):
    with pytest.raises(sc.VariancesError):
        sc.Variable(dims=(), variances=variance)


@pytest.mark.parametrize("unit", [
    sc.units.m,
    sc.Unit('m'), 'm',
    sc.Variable(dims=(), values=1.2, unit=sc.units.m).unit
])
def test_create_scalar_with_unit(unit):
    var = sc.Variable(dims=(), values=1.0, unit=unit)
    assert var.dims == []
    assert var.value == 1.0
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.m


def test_create_scalar_quantity():
    var = sc.Variable(dims=(), values=1.2, unit=sc.units.m)
    assert var.value == 1.2
    assert var.dims == []
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.m


def test_create_via_unit():
    expected = sc.Variable(dims=(), values=1.2, unit=sc.units.m)
    var = 1.2 * sc.units.m
    assert sc.identical(var, expected)


@pytest.mark.parametrize("dtype", (None, sc.dtype.Variable))
def test_create_scalar_dtype_Variable(dtype):
    elem = sc.Variable(dims=['x'], values=np.arange(4.0))
    var = sc.Variable(dims=(), values=elem, dtype=dtype)
    assert sc.identical(var.value, elem)
    assert var.dims == []
    assert var.dtype == sc.dtype.Variable
    assert var.unit == sc.units.dimensionless
    var = sc.Variable(dims=(), values=elem['x', 1:3], dtype=dtype)
    assert var.dtype == sc.dtype.Variable


@pytest.mark.parametrize("dtype", (None, sc.dtype.DataArray))
def test_create_scalar_dtype_DataArray(dtype):
    elem = sc.DataArray(data=sc.Variable(dims=['x'], values=np.arange(4.0)))
    var = sc.Variable(dims=(), values=elem, dtype=dtype)
    assert sc.identical(var.value, elem)
    assert var.dims == []
    assert var.dtype == sc.dtype.DataArray
    assert var.unit == sc.units.dimensionless
    var = sc.Variable(dims=(), values=elem['x', 1:3], dtype=dtype)
    assert var.dtype == sc.dtype.DataArray


@pytest.mark.parametrize("dtype", (None, sc.dtype.Dataset))
def test_create_scalar_dtype_Dataset(dtype):
    elem = sc.Dataset(data={'a': sc.Variable(dims=['x'], values=np.arange(4.0))})
    var = sc.Variable(dims=(), values=elem, dtype=dtype)
    assert sc.identical(var.value, elem)
    assert var.dims == []
    assert var.dtype == sc.dtype.Dataset
    assert var.unit == sc.units.dimensionless
    var = sc.Variable(dims=(), values=elem['x', 1:3], dtype=dtype)
    assert var.dtype == sc.dtype.Dataset


@pytest.mark.parametrize('value', (1, 1.2, True))
@pytest.mark.parametrize('dtype', (np.int32, np.int64, np.float32, np.float64, bool))
def test_create_scalar_conversion(value, dtype):
    converted = np.array(value, dtype=dtype).item()
    var = sc.Variable(dims=(), values=value, dtype=dtype)
    assert var.value == converted


@pytest.mark.parametrize('value', (1, 1.2, True))
@pytest.mark.parametrize('dtype', (sc.dtype.string, sc.dtype.Variable))
def test_create_scalar_invalid_conversion_numeric(value, dtype):
    with pytest.raises(ValueError):
        sc.Variable(dims=(), values=value, dtype=dtype)


@pytest.mark.parametrize(
    'dtype', (sc.dtype.int32, sc.dtype.int64, sc.dtype.float64, sc.dtype.Variable))
def test_create_scalar_invalid_conversion_str(dtype):
    with pytest.raises(ValueError):
        sc.Variable(dims=(), values='abc', dtype=dtype)


def test_create_1d_size_4():
    var = sc.Variable(dims=['x'], values=np.arange(4.0), unit=sc.units.m)
    assert var.shape == [4]
    np.testing.assert_array_equal(var.values, [0, 1, 2, 3])
    assert var.dims == ['x']
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.m


@pytest.mark.parametrize('values_type', (tuple, list, np.array))
@pytest.mark.parametrize('dtype_and_values',
                         ((sc.dtype.int64, [1, 2]), (sc.dtype.float64, [1.2, 3.4]),
                          (sc.dtype.bool, [True, False]),
                          (sc.dtype.string, ['a', 'bc'])))
def test_create_1d_dtype(values_type, dtype_and_values):
    def check(v):
        if dtype == sc.dtype.int64:
            # The dtype varies between Windows and Linux / MacOS.
            assert v.dtype in (sc.dtype.int32, sc.dtype.int64)
        else:
            assert v.dtype == dtype
        np.testing.assert_array_equal(v.values, values)

    dtype, values = dtype_and_values
    values = values_type(values)
    var = sc.Variable(dims=['x'], values=values)
    check(var)
    var = sc.Variable(dims=['x'], values=values, dtype=dtype)
    check(var)


def test_create_1d_dtype_precision():
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.int64))
    assert var.dtype == sc.dtype.int64
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.int32))
    assert var.dtype == sc.dtype.int32
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.float64))
    assert var.dtype == sc.dtype.float64
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.float32))
    assert var.dtype == sc.dtype.float32
    var = sc.Variable(dims=['x'], values=np.arange(4), dtype=np.dtype(np.float64))
    assert var.dtype == sc.dtype.float64
    var = sc.Variable(dims=['x'], values=np.arange(4), dtype=np.dtype(np.float32))
    assert var.dtype == sc.dtype.float32
    var = sc.Variable(dims=['x'], values=np.arange(4), dtype=np.dtype(np.int64))
    assert var.dtype == sc.dtype.int64
    var = sc.Variable(dims=['x'], values=np.arange(4), dtype=np.dtype(np.int32))
    assert var.dtype == sc.dtype.int32


@pytest.mark.parametrize('values_type', (tuple, list, np.array))
def test_create_1d_values_array_like(values_type):
    values = np.arange(5.0)
    var = sc.Variable(dims=['x'], values=values_type(values))
    assert var.dtype == sc.dtype.float64
    np.testing.assert_array_equal(var.values, values)
    assert var.variances is None


@pytest.mark.parametrize('variances_type', (tuple, list, np.array))
def test_create_1d_variances_array_like(variances_type):
    variances = np.arange(5.0)
    var = sc.Variable(dims=['x'], variances=variances_type(variances))
    assert var.dtype == sc.dtype.float64
    np.testing.assert_array_equal(var.values, np.zeros_like(variances))
    np.testing.assert_array_equal(var.variances, variances)


@pytest.mark.parametrize('values_type', (tuple, list, np.array))
@pytest.mark.parametrize('variances_type', (tuple, list, np.array))
def test_create_1d_values_and_variances_array_like(values_type, variances_type):
    values = np.arange(5.0)
    variances = np.arange(5.0) * 0.1
    var = sc.Variable(dims=['x'],
                      values=values_type(values),
                      variances=variances_type(variances))
    assert var.dtype == sc.dtype.float64
    np.testing.assert_array_equal(var.values, values)
    np.testing.assert_array_equal(var.variances, variances)


# The constructors in this test trigger deprecation warnings in numpy:
#
# Creating an ndarray from ragged nested sequences (which is a list-or-tuple
# of lists-or-tuples-or ndarrays with different lengths or shapes)
# is deprecated. If you meant to do this, you must specify 'dtype=object'
# when creating the ndarray.
#
# This is because the array-variable constructor uses py::array(values).
# Unfortunately, py::array_t<py::object> is not implemented.
# This test will fail when numpy changes the current behavior.
# Until then, we are fine.
#
# Users can avoid the issue by using numpy.ndarray for the 'values'.
@pytest.mark.parametrize('values_type',
                         (tuple, list, lambda x: np.array(x, dtype=object)))
def test_create_1d_dtype_object(values_type):
    def check(v):
        assert v.dtype == sc.dtype.PyObject
        # Cannot iterate over an ElementArrayView.
        assert v['x', 0].value == values[0]
        assert v['x', 1].value == values[1]

    values = values_type([{1, 2}, (3, 4)])
    var = sc.Variable(dims=['x'], values=values)
    check(var)
    var = sc.Variable(dims=['x'], values=values, dtype=sc.dtype.PyObject)
    check(var)


@pytest.mark.parametrize('dtype', (int, float, bool))
@pytest.mark.parametrize('values', ([1, 2], [1.2, 3.4], [True, False]))
def test_create_1d_override_dtype(dtype, values):
    var = sc.Variable(dims=['x'], values=values, dtype=dtype)
    converted = np.array(values).astype(dtype)
    scipp_dtype = sc.Variable(dims=['x'], values=converted).dtype
    assert var.dtype == scipp_dtype
    np.testing.assert_array_equal(var.values, converted)


def test_create_1D_vector_3_float64():
    var = sc.vectors(dims=['x'], values=[[1, 2, 3], [4, 5, 6]], unit=sc.units.m)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == ['x']
    assert var.dtype == sc.dtype.vector_3_float64
    assert var.unit == sc.units.m


def test_create_1d_bad_dims():
    with pytest.raises(ValueError):
        sc.Variable(dims=['x', 'y'], values=[1, 2])


def test_create_2d_inner_size_3():
    var = sc.Variable(dims=['x', 'y'],
                      values=np.arange(6.0).reshape(2, 3),
                      unit=sc.units.m)
    assert var.shape == [2, 3]
    np.testing.assert_array_equal(var.values[0], [0, 1, 2])
    np.testing.assert_array_equal(var.values[1], [3, 4, 5])
    assert var.dims == ['x', 'y']
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.m


@pytest.mark.parametrize('inner_values_type', (tuple, list, np.array))
@pytest.mark.parametrize('outer_values_type', (tuple, list, np.array))
def test_create_2d_values_array_like(inner_values_type, outer_values_type):
    values = np.arange(15.0).reshape(3, 5)
    expected = sc.Variable(dims=('x', 'y'), values=values)

    var = sc.Variable(dims=['x', 'y'],
                      values=outer_values_type(
                          [inner_values_type(val) for val in values]))
    assert sc.identical(var, expected)


@pytest.mark.parametrize('inner_variances_type', (tuple, list, np.array))
@pytest.mark.parametrize('outer_variances_type', (tuple, list, np.array))
def test_create_2d_variances_array_like(inner_variances_type, outer_variances_type):
    variances = np.arange(15.0).reshape(3, 5)
    var = sc.Variable(dims=['x', 'y'],
                      variances=outer_variances_type(
                          [inner_variances_type(val) for val in variances]))
    np.testing.assert_array_equal(var.values, np.zeros_like(variances))
    np.testing.assert_array_equal(var.variances, variances)


@pytest.mark.parametrize('inner_values_type', (tuple, list, np.array))
@pytest.mark.parametrize('outer_values_type', (tuple, list, np.array))
@pytest.mark.parametrize('inner_variances_type', (tuple, list, np.array))
@pytest.mark.parametrize('outer_variances_type', (tuple, list, np.array))
def test_create_2d_values_and_variances_array_like(inner_values_type, outer_values_type,
                                                   inner_variances_type,
                                                   outer_variances_type):
    values = np.arange(15.0).reshape(3, 5)
    variances = np.arange(15.0).reshape(3, 5) + 15
    expected = sc.Variable(dims=('x', 'y'), values=values, variances=variances)

    var = sc.Variable(
        dims=['x', 'y'],
        values=outer_values_type([inner_values_type(val) for val in values]),
        variances=outer_variances_type([inner_variances_type(val)
                                        for val in variances]))
    assert sc.identical(var, expected)
