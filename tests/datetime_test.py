# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

import itertools
import re

import numpy as np
import pytest

import scipp as sc

_UNIT_STRINGS = ('s', 'ms', 'us', 'ns', 'D', 'M', 'Y')


def _mismatch_pairs(units):
    yield from filter(lambda t: t[0] != t[1], itertools.product(units, units))


def _make_arrays(units, num_arrays, minsize=1):
    size = np.random.randint(minsize, minsize + 5)
    units = units if isinstance(units, tuple | list) else [units] * num_arrays
    res = [
        np.array(
            [np.datetime64(np.random.randint(0, 100000), unit) for _ in range(size)]
        )
        for unit in units
    ]
    if num_arrays == 1:
        return res[0]
    return res


def _make_datetimes(units, num):
    units = units if isinstance(units, tuple | list) else [units] * num
    res = [np.datetime64(np.random.randint(0, 1000), unit) for unit in units]
    if num == 1:
        return res[0]
    return res


@pytest.mark.parametrize("unit", _UNIT_STRINGS)
def test_construct_0d_datetime(unit) -> None:
    dtype = f'datetime64[{unit}]'
    value = _make_datetimes(unit, 1)
    # with value
    for var in (
        sc.Variable(dims=(), dtype=dtype, unit=unit, values=value),
        sc.Variable(dims=(), unit=unit, values=value),
        sc.Variable(dims=(), dtype=dtype, values=value),
        sc.Variable(dims=(), values=value),
    ):
        assert var.dtype == sc.DType.datetime64
        assert var.unit == unit
        assert var.value.dtype == dtype
        assert var.value == value


def test_construct_0d_datetime_array() -> None:
    var = sc.Variable(dims=(), values=np.array(np.datetime64(123, 's')))
    assert var.dtype == sc.DType.datetime64
    assert var.unit == sc.units.s
    assert var.value == np.datetime64(123, 's')


@pytest.mark.parametrize("unit", _UNIT_STRINGS)
def test_construct_0d_datetime_from_int(unit) -> None:
    value = np.random.randint(0, 1000)
    var = sc.Variable(dims=(), dtype=sc.DType.datetime64, unit=unit, values=value)
    assert var.dtype == sc.DType.datetime64
    assert var.unit == unit
    assert var.value.dtype == f'datetime64[{unit}]'
    assert var.value == np.datetime64(value, unit)


@pytest.mark.parametrize(('unit1', 'unit2'), _mismatch_pairs(_UNIT_STRINGS))
def test_construct_0d_datetime_mismatch(unit1, unit2) -> None:
    with pytest.raises(ValueError, match='unit encoded in the dtype'):
        sc.scalar(1, unit=unit1, dtype=f'datetime64[{unit2}]')


@pytest.mark.parametrize(('unit1', 'unit2'), _mismatch_pairs(('s', 'ms', 'us', 'ns')))
def test_construct_0d_datetime_unit_conversion(unit1, unit2) -> None:
    value = np.datetime64(2315169201, unit1)
    expected = sc.to_unit(sc.Variable(dims=(), values=value), unit2)
    assert sc.identical(sc.Variable(dims=(), values=value, unit=unit2), expected)
    assert sc.identical(
        sc.Variable(dims=(), values=value, dtype=f'datetime64[{unit2}]'), expected
    )


def test_construct_0d_datetime_nounit() -> None:
    # Can make a datetime variable without unit but cannot do anything
    # with it except set its unit.
    var = sc.Variable(dims=(), values=1, dtype=sc.DType.datetime64)
    assert var.dtype == sc.DType.datetime64
    assert var.unit == sc.units.one
    with pytest.raises(sc.UnitError):
        str(var)
    with pytest.raises(TypeError):
        var.value

    var.unit = sc.units.s
    value = np.datetime64(123, 's')
    var.value = value
    assert sc.identical(var, sc.Variable(dims=(), values=value))


@pytest.mark.parametrize("unit", _UNIT_STRINGS)
def test_0d_datetime_setter(unit) -> None:
    initial, replacement = _make_datetimes(unit, 2)
    var = sc.Variable(dims=(), values=initial)
    var.value = replacement
    assert sc.identical(var, sc.Variable(dims=(), values=replacement))


@pytest.mark.parametrize(('unit1', 'unit2'), _mismatch_pairs(_UNIT_STRINGS))
def test_0d_datetime_setter_mismatch(unit1, unit2) -> None:
    initial, replacement = _make_datetimes((unit1, unit2), 2)
    var1 = sc.Variable(dims=(), values=initial)
    with pytest.raises(ValueError, match='unit'):
        var1.value = replacement


@pytest.mark.parametrize("unit", _UNIT_STRINGS)
def test_construct_datetime(unit) -> None:
    dtype = f'datetime64[{unit}]'
    values = _make_arrays(unit, 1)
    # with values
    for var in (
        sc.Variable(dims=['x'], dtype=dtype, unit=unit, values=values),
        sc.Variable(dims=['x'], unit=unit, values=values),
        sc.Variable(dims=['x'], dtype=dtype, values=values),
        sc.Variable(dims=['x'], values=values),
    ):
        assert var.dims == ('x',)
        assert str(var.dtype) == 'datetime64'
        assert var.unit == unit
        assert var.values.dtype == dtype
        np.testing.assert_array_equal(var.values, values)


@pytest.mark.parametrize("unit", _UNIT_STRINGS)
def test_construct_datetime_from_int(unit) -> None:
    values = np.random.randint(0, 1000, np.random.randint(5, 100))
    var = sc.Variable(dims=['x'], dtype=sc.DType.datetime64, unit=unit, values=values)
    dtype_str = f'datetime64[{unit}]'
    assert var.dtype == sc.DType.datetime64
    assert var.unit == unit
    assert var.values.dtype == dtype_str
    np.testing.assert_array_equal(var.values, values.astype(dtype_str))


@pytest.mark.parametrize(('unit1', 'unit2'), _mismatch_pairs(_UNIT_STRINGS))
def test_construct_datetime_mismatch(unit1, unit2) -> None:
    with pytest.raises(ValueError, match='unit encoded in the dtype'):
        sc.array(dims=['x'], values=[1], unit=unit1, dtype=f'datetime64[{unit2}]')


@pytest.mark.parametrize(('unit1', 'unit2'), _mismatch_pairs(('s', 'ms', 'us', 'ns')))
def test_construct_datetime_unit_conversion(unit1, unit2) -> None:
    values = _make_arrays(unit1, 1)
    expected = sc.to_unit(sc.Variable(dims=['x'], values=values), unit2)
    assert sc.identical(sc.Variable(dims=['x'], values=values, unit=unit2), expected)
    assert sc.identical(
        sc.Variable(dims=['x'], values=values, dtype=f'datetime64[{unit2}]'), expected
    )


def test_construct_datetime_nounit() -> None:
    # Can make a datetime variable without unit but cannot do anything
    # with it except set its unit.
    var = sc.Variable(dims=['x'], values=[1, 2], dtype=sc.DType.datetime64)
    assert var.dtype == sc.DType.datetime64
    assert var.unit == sc.units.one
    with pytest.raises(sc.UnitError):
        str(var)
    with pytest.raises(TypeError):
        var.values

    var.unit = sc.units.s
    values = np.array([np.datetime64(123, 's'), np.datetime64(456, 's')])
    var.values = values
    assert sc.identical(var, sc.Variable(dims=['x'], values=values))


@pytest.mark.parametrize("unit", _UNIT_STRINGS)
def test_datetime_setter(unit) -> None:
    initial, replacement = _make_arrays(unit, 2)
    var = sc.Variable(dims=['x'], values=initial)
    var.values = replacement
    np.testing.assert_array_equal(var.values, replacement)
    assert sc.identical(var, sc.Variable(dims=['x'], values=replacement))


@pytest.mark.parametrize(('unit1', 'unit2'), _mismatch_pairs(_UNIT_STRINGS))
def test_datetime_setter_mismatch(unit1, unit2) -> None:
    initial, replacement = _make_arrays((unit1, unit2), 2)
    var1 = sc.Variable(dims=['x'], values=initial)
    with pytest.raises(ValueError, match='unit'):
        var1.values = replacement


@pytest.mark.parametrize("unit", _UNIT_STRINGS)
def test_datetime_slicing(unit) -> None:
    values1, values2 = _make_arrays(unit, 2, minsize=4)
    var = sc.Variable(dims=['x'], values=values1)
    for i in range(len(values1)):
        assert sc.identical(var['x', i], sc.Variable(dims=(), values=values1[i]))
    for i in range(len(values1) - 2):
        for j in range(i + 1, len(values1)):
            assert sc.identical(
                var['x', i:j], sc.Variable(dims=['x'], values=values1[i:j])
            )

    for i in range(len(values1)):
        var['x', i] = values2[i] * sc.Unit(unit)
    assert sc.identical(var, sc.Variable(dims=['x'], values=values2))

    var['x', 1:4] = sc.Variable(dims=['x'], values=values1[1:4])
    values2[1:4] = values1[1:4]
    assert sc.identical(var, sc.Variable(dims=['x'], values=values2))


def test_datetime_operations() -> None:
    dt = np.datetime64('now', 'ns')
    values = np.array([dt, dt + 123456789])
    var = sc.Variable(dims=['x'], values=values)

    res = var + 1 * sc.Unit('ns')
    assert str(res.dtype) == 'datetime64'
    assert res.unit == sc.units.ns
    np.testing.assert_array_equal(res.values, values + 1)

    shift = np.random.randint(0, 100, len(values))
    res = var - (var - sc.Variable(dims=['x'], values=shift, unit=sc.units.ns))
    assert res.dtype == sc.DType.int64
    assert res.unit == sc.units.ns
    np.testing.assert_array_equal(res.values, shift)


def test_datetime_operations_mismatch() -> None:
    dt = np.datetime64('now', 'ns')
    values = np.array([dt, dt + 123456789])
    var = sc.Variable(dims=['x'], values=values)

    with pytest.raises(RuntimeError):
        var + 1 * sc.Unit('s')
    with pytest.raises(RuntimeError):
        var + sc.Variable(
            dims=['x'], values=np.random.randint(0, 100, len(values)), unit=sc.units.us
        )


def fmt(time_point, unit):
    np_unit = 'm' if unit == 'min' else unit
    var = sc.scalar(time_point.astype(f'datetime64[{np_unit}]'), unit=unit)
    match = re.search(r'[\dT\-:\.]+$', str(var))
    assert match
    return match[0]


def test_datetime_formatting() -> None:
    dt = np.datetime64('1991-08-16T12:23:45.678901', 'us')
    assert fmt(dt, 'us') == '1991-08-16T12:23:45.678901'
    assert fmt(dt, 'ms') == '1991-08-16T12:23:45.678'
    assert fmt(dt, 's') == '1991-08-16T12:23:45'
    assert fmt(dt, 'min') == '1991-08-16T12:23'
    assert fmt(dt, 'h') == '1991-08-16T12'
    assert fmt(dt, 'D') == '1991-08-16'
    assert fmt(dt, 'M') == '1991-08'
    assert fmt(dt, 'Y') == '1991'
