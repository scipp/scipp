# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import numpy as np

import scipp as sc


def test_operation_with_python_float() -> None:
    a = sc.scalar(1.23456789)
    b = 9.87654321
    assert (a / b).value == (a / sc.scalar(b)).value


def test_operation_with_numpy_float32() -> None:
    result = sc.scalar(10) + np.float32(1.5)
    assert sc.identical(result, sc.scalar(11.5))


def test_reverse_operation_with_numpy_float32() -> None:
    result = np.float32(1.5) + sc.scalar(10)
    assert sc.identical(result, sc.scalar(11.5))


def test_inplace_operation_with_python_float() -> None:
    a = sc.scalar(1.23456789)
    b = 9.87654321
    expected = a.value / b
    a /= b
    assert a.value == expected


def test_inplace_operation_with_numpy_float32() -> None:
    a = sc.scalar(10.0)
    a += np.float32(1.5)
    assert sc.identical(a, sc.scalar(11.5))


def test_comparison_with_numpy_float32() -> None:
    assert sc.identical(sc.scalar(1.2) < np.float32(1.5), sc.scalar(True))


def test_operation_with_python_int32() -> None:
    a = sc.scalar(3, dtype='int32')
    b = 2
    assert sc.identical(a + b, a + sc.scalar(b))


def test_reverse_operation_with_python_int32() -> None:
    a = sc.scalar(3, dtype='int32')
    b = 2
    assert sc.identical(b + a, sc.scalar(b) + a)
    assert sc.identical(b - a, sc.scalar(b) - a)
    assert sc.identical(b * a, sc.scalar(b) * a)
    assert sc.identical(b / a, sc.scalar(b) / a)


def test_inplace_operation_with_python_int32() -> None:
    a = sc.scalar(3, dtype='int32')
    b = 2
    expected = a.value + b
    a += b
    assert a.value == expected
