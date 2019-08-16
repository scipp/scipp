# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sp
from scipp import Dim
import numpy as np
import operator


def test_type():
    variable_slice = sp.Variable(
        [Dim.X], np.arange(1, 10, dtype=float))[Dim.X, :]
    assert type(variable_slice) == sp.VariableProxy


def apply_test_op(op, a, b, data):
    op(a, b)
    # Assume numpy operations are correct as comparitor
    op(data, b.values)
    assert np.array_equal(a.values, data)


def test_binary_operations():
    _a = sp.Variable([Dim.X], np.arange(1, 10, dtype=float))
    _b = sp.Variable([Dim.X], np.arange(1, 10, dtype=float))
    a = _a[Dim.X, :]
    b = _b[Dim.X, :]

    data = np.copy(a.values)
    c = a + b
    assert type(c) == sp.Variable
    assert np.array_equal(c.values, data + data)
    c = a - b
    assert np.array_equal(c.values, data - data)
    c = a * b
    assert np.array_equal(c.values, data * data)
    c = a / b
    assert np.array_equal(c.values, data / data)

    apply_test_op(operator.iadd, a, b, data)
    apply_test_op(operator.isub, a, b, data)
    apply_test_op(operator.imul, a, b, data)
    apply_test_op(operator.itruediv, a, b, data)


def test_binary_float_operations():
    _a = sp.Variable([Dim.X], np.arange(1, 10, dtype=float))
    a = _a[Dim.X, :]
    data = np.copy(a.values)
    c = a + 2.0
    assert np.array_equal(c.values, data + 2.0)
    c = a - 2.0
    assert np.array_equal(c.values, data - 2.0)
    c = a * 2.0
    assert np.array_equal(c.values, data * 2.0)
    c = a / 2.0
    assert np.array_equal(c.values, data / 2.0)
    c = 2.0 + a
    assert np.array_equal(c.values, data + 2.0)
    c = 2.0 - a
    assert np.array_equal(c.values, 2.0 - data)
    c = 2.0 * a
    assert np.array_equal(c.values, data * 2.0)


def test_equal_not_equal():
    _a = sp.Variable([Dim.X], np.arange(1, 10, dtype=float))
    _b = sp.Variable([Dim.X], np.arange(1, 10, dtype=float))
    a = _a[Dim.X, :]
    b = _b[Dim.X, :]
    c = a + 2.0
    assert a == b
    assert b == a
    assert a != c
    assert c != a


def test_correct_temporaries():
    v = sp.Variable([Dim.X], values=np.arange(100.0))
    b = sp.sqrt(v)[Dim.X, 0:10]
    assert len(b.values) == 10
    b = b[Dim.X, 2:5]
    assert len(b.values) == 3


def test_set_variance():
    ds = sp.Dataset({
        'a': sp.Variable(dims=[Dim.X, Dim.Y], values=np.random.rand(2, 3)),
        'b': sp.Variable(dims=[Dim.X, Dim.Y], values=np.random.rand(2, 3))
    })
    var1 = ds['a']
    var2 = ds['b']
    vr = np.arange(6).reshape(2, 3)
    var1.variances = vr
    assert (var1.variances == vr).sum() == 6
    var2.variances = var1.variances
    assert (var2.variances == vr).sum() == 6
