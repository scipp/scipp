# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import numpy as np
import pytest
import scipp as sc


def test_midpoint_numpy():
    a = np.arange(10)
    low = a[:-1]
    high = a[1:]
    m = sc.midpoint(low, high)
    np.testing.assert_allclose(m, (low + high) / 2)


def test_midpoint_variable():
    v = sc.Variable(dims=['x'], values=np.arange(10), unit=sc.units.m)
    low = v['x', :-1]
    high = v['x', 1:]
    m = sc.midpoint(low, high)
    np.testing.assert_allclose(m.values, (low.values + high.values) / 2)
    assert m.unit == v.unit


def test_midpoint_mixed():
    v = sc.Variable(dims=['x'], values=np.arange(9), unit=sc.units.m)
    a = np.arange(9) + 1
    with pytest.raises(TypeError):
        sc.midpoint(v, a)


def test_midpoint_array_datetime():
    a = np.array([np.datetime64(i, 's') for i in range(0, 10, 2)])
    low = a[:-1]
    high = a[1:]
    m = sc.midpoint(low, high)
    np.testing.assert_array_equal(m, low + np.timedelta64(1, 's'))
    assert m.dtype == a.dtype


def test_midpoint_variable_datetime():
    a = np.array([np.datetime64(i, 's') for i in range(0, 10, 2)])
    v = sc.Variable(dims=['x'], values=a, unit=sc.units.s)
    low = v['x', :-1]
    high = v['x', 1:]
    m = sc.midpoint(low, high)
    assert sc.is_equal(m, low + 1*sc.units.s)
