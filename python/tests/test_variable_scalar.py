# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc


def test_scalar_Variable_values_property_float():
    var = sc.Variable(value=1.0, variance=2.0)
    assert var.dtype == sc.dtype.float64
    assert var.values == 1.0
    assert var.variances == 2.0


def test_scalar_Variable_values_property_int():
    var = sc.Variable(value=1)
    assert var.dtype == sc.dtype.int64
    assert var.values == 1


def test_scalar_Variable_values_property_string():
    var = sc.Variable(value='abc')
    assert var.dtype == sc.dtype.string
    assert var.values == 'abc'


def test_scalar_Variable_values_property_PyObject():
    var = sc.Variable(value=[1, 2])
    assert var.dtype == sc.dtype.PyObject
    assert var.values == [1, 2]
