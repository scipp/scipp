# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest
import scipp as sc


def test_scalar_Variable_values_property_float():
    var = sc.scalar(value=1.0, variance=2.0)
    assert var.dtype == sc.DType.float64
    assert var.values == 1.0
    assert var.variances == 2.0


def test_scalar_Variable_values_property_int():
    var = sc.scalar(1)
    assert var.dtype == sc.DType.int64
    assert var.values == 1


def test_scalar_Variable_values_property_string():
    var = sc.scalar('abc')
    assert var.dtype == sc.DType.string
    assert var.values == 'abc'


def test_scalar_Variable_values_property_PyObject():
    var = sc.scalar([1, 2])
    assert var.dtype == sc.DType.PyObject
    assert var.values == [1, 2]


@pytest.mark.parametrize(
    'var', (sc.scalar(3.1), sc.scalar(-2), sc.scalar('abc'), sc.scalar([1, 2])))
def test_scalar_Variable_value_is_same_as_values(var):
    assert var.value == var.values
