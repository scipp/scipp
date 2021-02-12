# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Jan-Lukas Wynen

from datetime import datetime, timedelta, timezone
import itertools
import re

import numpy as np
import pytest

import scipp as sc


def test_construct_0d_datetime():
    for unit in ('s', 'ms', 'us', 'ns'):
        dtype = f'datetime64[{unit}]'
        value = np.datetime64(2431, unit)
        # with value
        for var in (sc.Variable(dtype=dtype, unit=unit, value=value),
                    sc.Variable(unit=unit, value=value),
                    sc.Variable(dtype=dtype,
                                value=value), sc.Variable(value=value)):
            assert str(var.dtype) == 'datetime64'
            assert var.unit == unit
            assert var.value.dtype == dtype
            assert var.value == value
        # default init
        for var in (sc.Variable(dtype=dtype,
                                unit=unit), sc.Variable(dtype=dtype)):
            assert str(var.dtype) == 'datetime64'
            assert var.unit == unit
            assert var.value.dtype == dtype


def test_construct_0d_datetime_mismatch():
    for unit1, unit2 in filter(
            lambda t: t[0] != t[1],
            itertools.product(('s', 'ms', 'us', 'ns'), ('s', 'ms', 'us', 'ns'))):
        with pytest.raises(ValueError):
            sc.Variable(unit=unit1, dtype=f'datetime64[{unit2}]')
        with pytest.raises(RuntimeError):
            sc.Variable(value=np.datetime64('now', unit1),
                        dtype=f'datetime64[{unit2}]')
        with pytest.raises(RuntimeError):
            sc.Variable(value=np.datetime64('now', unit1), unit=unit2)


def test_0d_datetime_setter():
    for unit in ('s', 'ms', 'us', 'ns'):
        initial = np.datetime64(np.random.randint(0, 1000), unit)
        value = np.datetime64(np.random.randint(0, 1000), unit)
        var = sc.Variable(value=initial)
        var.value = value
        assert var.value == value
        assert sc.is_equal(var, sc.Variable(value=value))


def test_0d_datetime_setter_mismatch():
    for unit1, unit2 in filter(
            lambda t: t[0] != t[1],
            itertools.product(('s', 'ms', 'us', 'ns'), ('s', 'ms', 'us', 'ns'))):
        initial = np.datetime64(np.random.randint(0, 1000), unit1)
        value = np.datetime64(np.random.randint(0, 1000), unit2)
        var1 = sc.Variable(value=initial)
        with pytest.raises(ValueError):
            var1.value = value


def test_construct_datetime():
    for unit in ('s', 'ms', 'us', 'ns'):
        dtype = f'datetime64[{unit}]'
        values = np.array([
            np.datetime64(np.random.randint(0, 1000), unit)
            for _ in range(np.random.randint(1, 5))
        ])
        shape = [len(values)]
        # with values
        for var in (sc.Variable(dims=['x'],
                                dtype=dtype,
                                unit=unit,
                                values=values),
                    sc.Variable(dims=['x'], unit=unit, values=values),
                    sc.Variable(dims=['x'], dtype=dtype, values=values),
                    sc.Variable(dims=['x'], values=values)):
            assert var.dims == ['x']
            assert str(var.dtype) == 'datetime64'
            assert var.unit == unit
            assert var.values.dtype == dtype
            np.testing.assert_array_equal(var.values, values)
        # default init
        for var in (sc.Variable(dims=['x'],
                                shape=shape,
                                dtype=dtype,
                                unit=unit),
                    sc.Variable(dims=['x'], shape=shape, dtype=dtype)):
            assert var.dims == ['x']
            assert var.shape == shape
            assert str(var.dtype) == 'datetime64'
            assert var.unit == unit
            assert var.values.dtype == dtype


def test_construct_datetime_mismatch():
    for unit1, unit2 in filter(
            lambda t: t[0] != t[1],
            itertools.product(('s', 'ms', 'us', 'ns'), ('s', 'ms', 'us', 'ns'))):
        values = np.array([
            np.datetime64(np.random.randint(0, 1000), unit1)
            for _ in range(np.random.randint(1, 5))
        ])
        with pytest.raises(ValueError):
            sc.Variable(dims=['x'], unit=unit1, dtype=f'datetime64[{unit2}]')
        with pytest.raises(RuntimeError):
            sc.Variable(dims=['x'],
                        values=values,
                        dtype=f'datetime64[{unit2}]')
        with pytest.raises(RuntimeError):
            sc.Variable(dims=['x'], values=values, unit=unit2)


def test_datetime_setter():
    for unit in ('s', 'ms', 'us', 'ns'):
        size = np.random.randint(1, 5)
        initial = np.array([
            np.datetime64(np.random.randint(0, 1000), unit)
            for _ in range(size)
        ])
        values = np.array([
            np.datetime64(np.random.randint(0, 1000), unit)
            for _ in range(size)
        ])
        var = sc.Variable(dims=['x'], values=initial)
        var.values = values
        np.testing.assert_array_equal(var.values, values)
        assert sc.is_equal(var, sc.Variable(dims=['x'], values=values))


def test_datetime_setter_mismatch():
    for unit1, unit2 in filter(
            lambda t: t[0] != t[1],
            itertools.product(('s', 'ms', 'us', 'ns'), ('s', 'ms', 'us', 'ns'))):
        size = np.random.randint(1, 5)
        initial = np.array([
            np.datetime64(np.random.randint(0, 1000), unit1)
            for _ in range(size)
        ])
        values = np.array([
            np.datetime64(np.random.randint(0, 1000), unit2)
            for _ in range(size)
        ])
        var1 = sc.Variable(dims=['x'], values=initial)
        with pytest.raises(ValueError):
            var1.values = values


def test_datetime_slicing():
    for unit in ('s', 'ms', 'us', 'ns'):
        values = np.array([
            np.datetime64(np.random.randint(0, 1000), unit)
            for _ in range(np.random.randint(4, 6))
        ])
        var = sc.Variable(dims=['x'], values=values)
        for i in range(len(values)):
            assert sc.is_equal(var['x', i], sc.Variable(value=values[i]))
        for i in range(len(values) - 2):
            for j in range(i + 1, len(values)):
                assert sc.is_equal(var['x', i:j],
                                   sc.Variable(dims=['x'], values=values[i:j]))

        values2 = np.array([
            np.datetime64(np.random.randint(0, 1000), unit)
            for _ in range(len(values))
        ])
        for i in range(len(values)):
            var['x', i] = values2[i] * sc.Unit(unit)
        assert sc.is_equal(var, sc.Variable(dims=['x'], values=values2))

        var['x', 1:4] = sc.Variable(dims=['x'], values=values[1:4])
        values2[1:4] = values[1:4]
        assert sc.is_equal(var, sc.Variable(dims=['x'], values=values2))


def test_datetime_operations():
    dt = np.datetime64('now', 'ns')
    values = np.array([dt, dt + 123456789])
    var = sc.Variable(dims=['x'], values=values)

    res = var + 1 * sc.Unit('ns')
    assert str(res.dtype) == 'datetime64'
    assert res.unit == sc.units.ns
    np.testing.assert_array_equal(res.values, values + 1)

    shift = np.random.randint(0, 100, len(values))
    res = var - (var - sc.Variable(['x'], values=shift, unit=sc.units.ns))
    assert res.dtype == sc.dtype.int64
    assert res.unit == sc.units.ns
    np.testing.assert_array_equal(res.values, shift)


def test_datetime_operations_mismatch():
    dt = np.datetime64('now', 'ns')
    values = np.array([dt, dt + 123456789])
    var = sc.Variable(dims=['x'], values=values)

    with pytest.raises(RuntimeError):
        var + 1 * sc.Unit('s')
    with pytest.raises(RuntimeError):
        var + sc.Variable(['x'],
                          values=np.random.randint(0, 100, len(values)),
                          unit=sc.units.us)


def test_datetime_formatting():
    # Time since epoch for a totally arbitrary date.
    # The timezone has an offset of 0 to emulate a timestamp obtained from some source that
    # is not aware of timezones.
    timestamp = int(datetime(year=1991, month=8, day=16, hour=1, minute=2, second=3, microsecond=456789,
                             tzinfo=timezone(timedelta(hours=0))).timestamp() * 10**6)
    var = sc.scalar(np.datetime64(timestamp, 'us'))
    match = re.search(r'\[[\dT\-:\.]+]', str(var))
    assert match
    # Make sure that date and time are printed unchanged, i.e. there was no timezone conversion.
    assert match[0][1:-1] == "1991-08-16T01:02:03.456789"
