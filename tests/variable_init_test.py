# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

import platform
from typing import Any

import numpy as np
import pytest

import scipp as sc


def representation_of_native_int() -> sc.DType:
    if (
        platform.system() == 'Windows'
        and np.lib.NumpyVersion(np.__version__) < '2.0.0b1'
    ):
        return sc.DType.int32
    return sc.DType.int64


# Tuples (dtype, expected, val) where
# - dtype: Object to pass as `dtype` to functions
# - expected: dtype of the return value of the function
# - val: Value of a matching type
DTYPE_INPUT_TO_EXPECTED: tuple[
    tuple[type | sc.DType | np.dtype[Any], sc.DType, object], ...
] = (
    (int, representation_of_native_int(), 0),
    (float, sc.DType.float64, 1.2),
    (bool, sc.DType.bool, True),
    (str, sc.DType.string, 'abc'),
    (sc.DType.int32, sc.DType.int32, 2),
    (sc.DType.int64, sc.DType.int64, 3),
    (sc.DType.float32, sc.DType.float32, 4.5),
    (sc.DType.float64, sc.DType.float64, 5.6),
    (sc.DType.bool, sc.DType.bool, False),
    (sc.DType.string, sc.DType.string, 'def'),
    (sc.DType.datetime64, sc.DType.datetime64, 123),
    (sc.DType.PyObject, sc.DType.PyObject, {}),
    (np.int32, sc.DType.int32, 6),
    (np.int64, sc.DType.int64, 7),
    (np.float32, sc.DType.float32, 8.9),
    (np.float64, sc.DType.float64, 9.1),
    (np.dtype(bool), sc.DType.bool, True),
    (np.dtype(str), sc.DType.string, 'ghi'),
    (np.dtype('datetime64'), sc.DType.datetime64, 456),
    (np.dtype('datetime64[ns]'), sc.DType.datetime64, 789),
)


@pytest.mark.parametrize(
    "value",
    [1.2, np.float64(1.2), sc.Variable(dims=(), values=1.2).value, np.array(1.2)],
)
def test_create_scalar_with_float_value(value: object) -> None:
    var = sc.Variable(dims=(), values=value)
    assert var.value == value
    assert var.dims == ()
    assert var.ndim == 0
    assert var.dtype == sc.DType.float64
    assert var.unit == sc.units.dimensionless


@pytest.mark.parametrize(
    "variance",
    [1.2, np.float64(1.2), sc.Variable(dims=(), values=1.2).value, np.array(1.2)],
)
def test_create_scalar_with_float_variance(variance: object) -> None:
    var = sc.Variable(dims=(), variances=variance)
    assert var.value == 0.0
    assert var.variance == variance
    assert var.dims == ()
    assert var.ndim == 0
    assert var.dtype == sc.DType.float64
    assert var.unit == sc.units.dimensionless


@pytest.mark.parametrize(
    "value",
    [1.2, np.float64(1.2), sc.Variable(dims=(), values=1.2).value, np.array(1.2)],
)
@pytest.mark.parametrize(
    "variance",
    [3.4, np.float64(3.4), sc.Variable(dims=(), values=3.4).value, np.array(3.4)],
)
def test_create_scalar_with_float_value_and_variance(
    value: object, variance: object
) -> None:
    var = sc.Variable(dims=(), values=value, variances=variance)
    assert var.value == value
    assert var.variance == variance
    assert var.dims == ()
    assert var.ndim == 0
    assert var.dtype == sc.DType.float64
    assert var.unit == sc.units.dimensionless


@pytest.mark.parametrize(
    'args', [(sc.DType.int64, 1), (sc.DType.bool, True), (sc.DType.string, 'a')]
)
def test_create_scalar_with_value(args: tuple[sc.DType, object]) -> None:
    dtype, value = args
    var = sc.Variable(dims=(), values=value)
    assert var.value == value
    assert var.dims == ()
    assert var.ndim == 0
    assert var.dtype == dtype
    if dtype in (sc.DType.string, sc.DType.bool):
        assert var.unit is None
    else:
        assert var.unit == sc.units.one


@pytest.mark.parametrize('args', [(sc.DType.bool, True), (sc.DType.string, 'a')])
def test_create_scalar_with_value_array(args: tuple[sc.DType, object]) -> None:
    dtype, value = args
    var = sc.Variable(dims=(), values=np.array(value))
    assert var.value == value
    assert var.dims == ()
    assert var.ndim == 0
    assert var.dtype == dtype
    if dtype in (sc.DType.string, sc.DType.bool):
        assert var.unit is None
    else:
        assert var.unit == sc.units.dimensionless


def test_create_scalar_with_value_array_int() -> None:
    var = sc.Variable(dims=(), values=np.array(2))
    assert var.value == 2
    assert var.dims == ()
    assert var.ndim == 0
    # The dtype varies between Windows and Linux / MacOS.
    assert var.dtype in (sc.DType.int32, sc.DType.int64)
    assert var.unit == sc.units.dimensionless


def test_create_scalar_numpy() -> None:
    v = sc.Variable(dims=['x'], values=np.array([0]), dtype=np.float32)
    var = v['x', 0].copy()
    assert sc.identical(var, sc.scalar(np.float32()))

    v = sc.Variable(dims=['x'], values=np.array([0]), dtype=np.float32)
    var = v['x', 0].copy()
    var.unit = sc.units.m
    assert sc.identical(var, np.float32(0.0) * sc.units.m)
    var.unit = sc.units.m ** (-1)
    assert sc.identical(var, np.float32(0.0) / sc.units.m)


@pytest.mark.parametrize('variance', [1, True, 'a', sc.Variable(dims=(), values=1.2)])
def test_create_scalar_invalid_variance(variance: object) -> None:
    with pytest.raises(sc.VariancesError):
        sc.Variable(dims=(), variances=variance)


@pytest.mark.parametrize(
    "unit",
    [
        sc.units.m,
        sc.Unit('m'),
        'm',
        sc.Variable(dims=(), values=1.2, unit=sc.units.m).unit,
    ],
)
def test_create_scalar_with_unit(unit: sc.Unit | str | None) -> None:
    var = sc.Variable(dims=(), values=1.0, unit=unit)
    assert var.dims == ()
    assert var.ndim == 0
    assert var.value == 1.0
    assert var.dtype == sc.DType.float64
    assert var.unit == sc.units.m


def test_create_scalar_quantity() -> None:
    var = sc.Variable(dims=(), values=1.2, unit=sc.units.m)
    assert var.value == 1.2
    assert var.dims == ()
    assert var.ndim == 0
    assert var.dtype == sc.DType.float64
    assert var.unit == sc.units.m


def test_create_via_unit() -> None:
    expected = sc.Variable(dims=(), values=1.2, unit=sc.units.m)
    var = 1.2 * sc.units.m
    assert sc.identical(var, expected)


def test_create_scalar_dtypes() -> None:
    for dtype, expected, val in DTYPE_INPUT_TO_EXPECTED:
        unit = 'ns' if expected == sc.DType.datetime64 else 'one'
        assert sc.scalar(val, dtype=dtype, unit=unit).dtype == expected


@pytest.mark.parametrize(
    "val_and_dtype",
    [
        (np.int32(2), 'int32'),
        (np.int64(42), 'int64'),
        (np.float32(5.4), 'float32'),
        (np.float64(9.9), 'float64'),
        (np.bool_(True), 'bool'),
    ],
)
def test_create_scalar_numpy_type(val_and_dtype: tuple[object, str]) -> None:
    val, dtype = val_and_dtype
    assert sc.scalar(val).dtype == dtype


@pytest.mark.parametrize("dtype", [None, sc.DType.Variable, sc.Variable])
def test_create_scalar_dtype_Variable(dtype: Any) -> None:
    elem = sc.Variable(dims=['x'], values=np.arange(4.0))
    var = sc.Variable(dims=(), values=elem, dtype=dtype)
    assert sc.identical(var.value, elem)
    assert var.dims == ()
    assert var.ndim == 0
    assert var.dtype == sc.DType.Variable
    assert var.unit is None
    var = sc.Variable(dims=(), values=elem['x', 1:3], dtype=dtype)
    assert var.dtype == sc.DType.Variable


@pytest.mark.parametrize("dtype", [None, sc.DType.DataArray, sc.DataArray])
def test_create_scalar_dtype_DataArray(dtype: Any) -> None:
    elem = sc.DataArray(data=sc.Variable(dims=['x'], values=np.arange(4.0)))
    var = sc.Variable(dims=(), values=elem, dtype=dtype)
    assert sc.identical(var.value, elem)
    assert var.dims == ()
    assert var.ndim == 0
    assert var.dtype == sc.DType.DataArray
    assert var.unit is None
    var = sc.Variable(dims=(), values=elem['x', 1:3], dtype=dtype)
    assert var.dtype == sc.DType.DataArray


@pytest.mark.parametrize("dtype", [None, sc.DType.Dataset, sc.Dataset])
def test_create_scalar_dtype_Dataset(dtype: Any) -> None:
    elem = sc.Dataset(data={'a': sc.Variable(dims=['x'], values=np.arange(4.0))})
    var = sc.Variable(dims=(), values=elem, dtype=dtype)
    assert sc.identical(var.value, elem)
    assert var.dims == ()
    assert var.ndim == 0
    assert var.dtype == sc.DType.Dataset
    assert var.unit is None
    var = sc.Variable(dims=(), values=elem['x', 1:3], dtype=dtype)
    assert var.dtype == sc.DType.Dataset


@pytest.mark.parametrize(
    "dtype",
    [
        1,
        2.3,
        'string',
        sc.scalar(0),
        sc.DataArray(sc.scalar(1)),
        sc.Dataset({'a': sc.scalar(3)}),
    ],
)
def test_create_scalar_with_instance_as_dtype(dtype: Any) -> None:
    with pytest.raises(TypeError):
        sc.Variable(dims=(), values=dtype, dtype=dtype)


@pytest.mark.parametrize("dtype", [str, sc.Variable, sc.DataArray, sc.Dataset])
def test_create_scalar_value_must_be_convertible_to_dtype(dtype: type) -> None:
    with pytest.raises(ValueError, match='Cannot convert values'):
        sc.Variable(dims=(), values=0, dtype=dtype)


@pytest.mark.parametrize('value', [1, 1.2, True])
@pytest.mark.parametrize('dtype', [np.int32, np.int64, np.float32, np.float64, bool])
def test_create_scalar_conversion(value: int | bool, dtype: type) -> None:
    converted: object = np.array(value, dtype=dtype).item()
    var = sc.Variable(dims=(), values=value, dtype=dtype)
    assert var.value == converted


@pytest.mark.parametrize('value', [1, 1.2, True])
@pytest.mark.parametrize('dtype', [sc.DType.string, sc.DType.Variable])
def test_create_scalar_invalid_conversion_numeric(
    value: int | bool, dtype: sc.DType
) -> None:
    with pytest.raises(ValueError, match='Cannot convert values'):
        sc.Variable(dims=(), values=value, dtype=dtype)


@pytest.mark.parametrize(
    'dtype', [sc.DType.int32, sc.DType.int64, sc.DType.float64, sc.DType.Variable]
)
def test_create_scalar_invalid_conversion_str(dtype: sc.DType) -> None:
    with pytest.raises(ValueError, match='Cannot convert values'):
        sc.Variable(dims=(), values='abc', dtype=dtype)


def test_create_1d_size_4() -> None:
    var = sc.Variable(dims=['x'], values=np.arange(4.0), unit=sc.units.m)
    assert var.shape == (4,)
    np.testing.assert_array_equal(var.values, [0, 1, 2, 3])
    assert var.dims == ('x',)
    assert var.ndim == 1
    assert var.dtype == sc.DType.float64
    assert var.unit == sc.units.m


@pytest.mark.parametrize('values_type', [tuple, list, np.array])
@pytest.mark.parametrize(
    'dtype_and_values',
    [
        (sc.DType.int64, [1, 2]),
        (sc.DType.float64, [1.2, 3.4]),
        (sc.DType.bool, [True, False]),
        (sc.DType.string, ['a', 'bc']),
    ],
)
def test_create_1d_dtype(
    values_type: type, dtype_and_values: tuple[sc.DType, list[Any]]
) -> None:
    def check(v: sc.Variable) -> None:
        if dtype == sc.DType.int64:
            # The dtype varies between Windows and Linux / MacOS.
            assert v.dtype in (sc.DType.int32, sc.DType.int64)
        else:
            assert v.dtype == dtype
        np.testing.assert_array_equal(v.values, values)

    dtype, values = dtype_and_values
    values = values_type(values)
    var = sc.Variable(dims=['x'], values=values)
    check(var)
    var = sc.Variable(dims=['x'], values=values, dtype=dtype)
    check(var)


def test_create_1d_dtype_precision() -> None:
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.int64))
    assert var.dtype == sc.DType.int64
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.int32))
    assert var.dtype == sc.DType.int32
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.float64))
    assert var.dtype == sc.DType.float64
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.float32))
    assert var.dtype == sc.DType.float32
    var = sc.Variable(dims=['x'], values=np.arange(4), dtype=np.dtype(np.float64))
    assert var.dtype == sc.DType.float64
    var = sc.Variable(dims=['x'], values=np.arange(4), dtype=np.dtype(np.float32))
    assert var.dtype == sc.DType.float32
    var = sc.Variable(dims=['x'], values=np.arange(4), dtype=np.dtype(np.int64))
    assert var.dtype == sc.DType.int64
    var = sc.Variable(dims=['x'], values=np.arange(4), dtype=np.dtype(np.int32))
    assert var.dtype == sc.DType.int32


@pytest.mark.parametrize('values_type', [tuple, list, np.array])
def test_create_1d_values_array_like(values_type: type) -> None:
    values = np.arange(5.0)
    var = sc.Variable(dims=['x'], values=values_type(values))
    assert var.dtype == sc.DType.float64
    np.testing.assert_array_equal(var.values, values)
    assert var.variances is None


@pytest.mark.parametrize('variances_type', [tuple, list, np.array])
def test_create_1d_variances_array_like(variances_type: type) -> None:
    variances = np.arange(5.0)
    var = sc.Variable(dims=['x'], variances=variances_type(variances))
    assert var.dtype == sc.DType.float64
    np.testing.assert_array_equal(var.values, np.zeros_like(variances))
    np.testing.assert_array_equal(var.variances, variances)


@pytest.mark.parametrize('values_type', [tuple, list, np.array])
@pytest.mark.parametrize('variances_type', [tuple, list, np.array])
def test_create_1d_values_and_variances_array_like(
    values_type: type, variances_type: type
) -> None:
    values = np.arange(5.0)
    variances = np.arange(5.0) * 0.1
    var = sc.Variable(
        dims=['x'], values=values_type(values), variances=variances_type(variances)
    )
    assert var.dtype == sc.DType.float64
    np.testing.assert_array_equal(var.values, values)
    np.testing.assert_array_equal(var.variances, variances)


def test_create_1d_dtype_object() -> None:
    def check(v: sc.Variable) -> None:
        assert v.dtype == sc.DType.PyObject
        # Cannot iterate over an ElementArrayView.
        assert v['x', 0].value == values[0]
        assert v['x', 1].value == values[1]

    values = np.array([{1, 2}, (3, 4)], dtype=object)
    var = sc.Variable(dims=['x'], values=values)
    check(var)
    var = sc.Variable(dims=['x'], values=values, dtype=sc.DType.PyObject)
    check(var)


@pytest.mark.parametrize('dtype', [int, float, bool])
@pytest.mark.parametrize('values', [[1, 2], [1.2, 3.4], [True, False]])
def test_create_1d_override_dtype(dtype: type, values: list[int | bool]) -> None:
    var = sc.Variable(dims=['x'], values=values, dtype=dtype)
    converted: Any = np.array(values).astype(dtype)
    scipp_dtype = sc.Variable(dims=['x'], values=converted).dtype
    assert var.dtype == scipp_dtype
    np.testing.assert_array_equal(var.values, converted)


def test_create_1D_vector3() -> None:
    var = sc.vectors(dims=['x'], values=[[1, 2, 3], [4, 5, 6]], unit=sc.units.m)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == ('x',)
    assert var.ndim == 1
    assert var.dtype == sc.DType.vector3
    assert var.unit == sc.units.m


def test_create_1d_bad_dims() -> None:
    with pytest.raises(ValueError, match='number of dimensions'):
        sc.Variable(dims=['x', 'y'], values=[1, 2])


def test_create_2d_inner_size_3() -> None:
    var = sc.Variable(
        dims=['x', 'y'], values=np.arange(6.0).reshape(2, 3), unit=sc.units.m
    )
    assert var.shape == (2, 3)
    np.testing.assert_array_equal(var.values[0], [0, 1, 2])
    np.testing.assert_array_equal(var.values[1], [3, 4, 5])
    assert var.dims == ('x', 'y')
    assert var.ndim == 2
    assert var.dtype == sc.DType.float64
    assert var.unit == sc.units.m


@pytest.mark.parametrize('inner_values_type', [tuple, list, np.array])
@pytest.mark.parametrize('outer_values_type', [tuple, list, np.array])
def test_create_2d_values_array_like(
    inner_values_type: type, outer_values_type: type
) -> None:
    values = np.arange(15.0).reshape(3, 5)
    expected = sc.Variable(dims=('x', 'y'), values=values)

    var = sc.Variable(
        dims=['x', 'y'],
        values=outer_values_type([inner_values_type(val) for val in values]),
    )
    assert sc.identical(var, expected)


@pytest.mark.parametrize('inner_variances_type', [tuple, list, np.array])
@pytest.mark.parametrize('outer_variances_type', [tuple, list, np.array])
def test_create_2d_variances_array_like(
    inner_variances_type: type, outer_variances_type: type
) -> None:
    variances = np.arange(15.0).reshape(3, 5)
    var = sc.Variable(
        dims=['x', 'y'],
        variances=outer_variances_type(
            [inner_variances_type(val) for val in variances]
        ),
    )
    np.testing.assert_array_equal(var.values, np.zeros_like(variances))
    np.testing.assert_array_equal(var.variances, variances)


@pytest.mark.parametrize('inner_values_type', [tuple, list, np.array])
@pytest.mark.parametrize('outer_values_type', [tuple, list, np.array])
@pytest.mark.parametrize('inner_variances_type', [tuple, list, np.array])
@pytest.mark.parametrize('outer_variances_type', [tuple, list, np.array])
def test_create_2d_values_and_variances_array_like(
    inner_values_type: type,
    outer_values_type: type,
    inner_variances_type: type,
    outer_variances_type: type,
) -> None:
    values = np.arange(15.0).reshape(3, 5)
    variances = np.arange(15.0).reshape(3, 5) + 15
    expected = sc.Variable(dims=('x', 'y'), values=values, variances=variances)

    var = sc.Variable(
        dims=['x', 'y'],
        values=outer_values_type([inner_values_type(val) for val in values]),
        variances=outer_variances_type(
            [inner_variances_type(val) for val in variances]
        ),
    )
    assert sc.identical(var, expected)
