# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import numpy as np
import scipp as sc


@pytest.mark.parametrize('dt', (sc.DType.int32, sc.DType.float64, sc.DType.string))
def test_dtype_comparison_equal(dt):
    assert dt == dt


@pytest.mark.parametrize('other', (sc.DType.int32, sc.DType.float64, sc.DType.string))
def test_dtype_comparison_not_equal(other):
    assert sc.DType.int64 != other


@pytest.mark.parametrize('name', ('int64', 'float32', 'str'))
def test_dtype_comparison_str(name):
    assert sc.DType(name) == name
    assert name == sc.DType(name)


def test_dtype_comparison_type():
    assert sc.DType.float64 == float
    assert float == sc.DType.float64
    assert sc.DType.string == str
    assert str == sc.DType.string
    # Depends on OS
    assert int in (sc.DType.int64, sc.DType.int32)


def test_numpy_comparison():
    assert sc.DType.int32 == np.dtype(np.int32)
    with pytest.raises(TypeError):
        # Calls np.DType.__eq__ which does not know how to interpret sc.DType
        assert np.dtype(np.int32) == sc.DType.int32


def test_dtype_string_construction():
    assert sc.DType('int64') == sc.DType.int64
    assert sc.DType('float64') == sc.DType.float64
    assert sc.DType('float') == sc.DType.float64
    assert sc.DType('str') == sc.DType.string


def test_dtype_type_class_construction():
    assert sc.DType(float) == sc.DType.float64
    assert sc.DType(str) == sc.DType.string
    # Depends on OS
    assert sc.DType(int) in (sc.DType.int64, sc.DType.int32)


def test_dtype_numpy_dtype_construction():
    assert sc.DType(np.dtype('float')) == sc.DType.float64
    assert sc.DType(np.dtype('int64')) == sc.DType.int64
    assert sc.DType(np.dtype('str')) == sc.DType.string


def test_dtype_numpy_element_type_construction():
    assert sc.DType(np.float64) == sc.DType.float64
    assert sc.DType(np.int32) == sc.DType.int32


def test_repr():
    assert repr(sc.DType('int32')) == "DType('int32')"
    assert repr(sc.DType('float')) == "DType('float64')"


def test_str():
    assert str(sc.DType('int32')) == 'int32'
    assert str(sc.DType('float')) == 'float64'


def test_predefined_dtypes_are_read_only():
    with pytest.raises(AttributeError):
        sc.DType.int64 = sc.DType('str')
