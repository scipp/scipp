# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


@pytest.mark.parametrize('dt', [sc.DType.int32, sc.DType.float64, sc.DType.string])
def test_dtype_comparison_equal(dt: sc.DType) -> None:
    assert dt == dt


@pytest.mark.parametrize('other', [sc.DType.int32, sc.DType.float64, sc.DType.string])
def test_dtype_comparison_not_equal(other: sc.DType) -> None:
    assert sc.DType.int64 != other


@pytest.mark.parametrize('name', ['int64', 'float32', 'str'])
def test_dtype_comparison_str(name: str) -> None:
    assert sc.DType(name) == name
    assert name == sc.DType(name)
    assert sc.DType(name) != 'bool'
    assert 'bool' != sc.DType(name)


def test_dtype_comparison_type() -> None:
    assert sc.DType.float64 == float  # noqa: E721
    assert float == sc.DType.float64  # noqa: E721
    assert sc.DType.string == str  # noqa: E721
    assert str == sc.DType.string  # noqa: E721
    # Depends on OS
    assert int in (sc.DType.int64, sc.DType.int32)

    assert sc.DType.float64 != int  # noqa: E721
    assert int != sc.DType.float64  # noqa: E721
    assert sc.DType.string != float  # noqa: E721
    assert float != sc.DType.string  # noqa: E721


def test_numpy_comparison() -> None:
    assert sc.DType.int32 == np.dtype(np.int32)
    assert sc.DType.int32 != np.dtype(np.int64)


def check_numpy_version_for_comparison() -> bool:
    major, minor, *_ = np.__version__.split('.')
    if int(major) == 1 and int(minor) < 21:
        return True
    return False


@pytest.mark.skipif(
    check_numpy_version_for_comparison(), reason='at least numpy 1.21 required'
)
def test_numpy_comparison_numpy_on_lhs() -> None:
    assert np.dtype(np.int32) == sc.DType.int32
    assert np.dtype(np.int32) != sc.DType.int64


def test_dtype_string_construction() -> None:
    assert sc.DType('int64') == sc.DType.int64
    assert sc.DType('float64') == sc.DType.float64
    assert sc.DType('float') == sc.DType.float64
    assert sc.DType('str') == sc.DType.string


def test_dtype_type_class_construction() -> None:
    assert sc.DType(float) == sc.DType.float64
    assert sc.DType(str) == sc.DType.string
    # Depends on OS
    assert sc.DType(int) in (sc.DType.int64, sc.DType.int32)


def test_dtype_numpy_dtype_construction() -> None:
    assert sc.DType(np.dtype('float')) == sc.DType.float64
    assert sc.DType(np.dtype('int64')) == sc.DType.int64
    assert sc.DType(np.dtype('str')) == sc.DType.string


def test_dtype_numpy_element_type_construction() -> None:
    assert sc.DType(np.float64) == sc.DType.float64
    assert sc.DType(np.int32) == sc.DType.int32


def test_repr() -> None:
    assert repr(sc.DType('int32')) == "DType('int32')"
    assert repr(sc.DType('float')) == "DType('float64')"


def test_str() -> None:
    assert str(sc.DType('int32')) == 'int32'
    assert str(sc.DType('float')) == 'float64'


def test_predefined_dtypes_are_read_only() -> None:
    with pytest.raises(AttributeError):
        sc.DType.int64 = sc.DType('str')
