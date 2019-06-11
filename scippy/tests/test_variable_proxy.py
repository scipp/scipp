# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scippy as sp
from scippy import Dim
import numpy as np
import operator


#def setUp():
#    self._a = sp.Variable([sp.Dim.X], np.arange(1, 10, dtype=float))
#    self._b = sp.Variable([sp.Dim.X], np.arange(1, 10, dtype=float))

def test_type():
    variable_slice = sp.Variable([Dim.X], np.arange(1, 10, dtype=float))[Dim.X, :]
    assert type(variable_slice) == sp.VariableProxy

def apply_test_op(op, a, b, data):
    op(a, b)
    # Assume numpy operations are correct as comparitor
    op(data, b.numpy)
    assert np.array_equal(a.numpy, data)

def test_binary_operations():
    _a = sp.Variable([Dim.X], np.arange(1, 10, dtype=float))
    _b = sp.Variable([Dim.X], np.arange(1, 10, dtype=float))
    a = _a[Dim.X, :]
    b = _b[Dim.X, :]

    data = np.copy(a.numpy)
    c = a + b
    assert type(c) == sp.Variable
    assert np.array_equal(c.numpy, data + data)
    c = a - b
    assert np.array_equal(c.numpy, data - data)
    c = a * b
    assert np.array_equal(c.numpy, data * data)
    c = a / b
    assert np.array_equal(c.numpy, data / data)

    apply_test_op(operator.iadd, a, b, data)
    apply_test_op(operator.isub, a, b, data)
    apply_test_op(operator.imul, a, b, data)
    apply_test_op(operator.itruediv, a, b, data)

def test_binary_float_operations():
    _a = sp.Variable([Dim.X], np.arange(1, 10, dtype=float))
    a = _a[Dim.X, :]
    data = np.copy(a.numpy)
    c = a + 2.0
    assert np.array_equal(c.numpy, data + 2.0)
    c = a - 2.0
    assert np.array_equal(c.numpy, data - 2.0)
    c = a * 2.0
    assert np.array_equal(c.numpy, data * 2.0)
    c = a / 2.0
    assert np.array_equal(c.numpy, data / 2.0)
    c = 2.0 + a
    assert np.array_equal(c.numpy, data + 2.0)
    c = 2.0 - a
    assert np.array_equal(c.numpy, 2.0 - data)
    c = 2.0 * a
    assert np.array_equal(c.numpy, data * 2.0)

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
