# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import operator
from collections.abc import Callable
from typing import Any

import numpy as np
import numpy.typing as npt

import scipp as sc


def test_type() -> None:
    variable_slice = sc.Variable(dims=['x'], values=np.arange(1, 10, dtype=float))[
        'x', :
    ]
    assert type(variable_slice) is sc.Variable


def test_astype() -> None:
    variable_slice = sc.Variable(dims=['x'], values=np.arange(1, 10, dtype=np.int64))[
        'x', :
    ]
    assert variable_slice.dtype == sc.DType.int64

    var_as_float = variable_slice.astype(sc.DType.float32)
    assert var_as_float.dtype == sc.DType.float32


def apply_inplace_binary_operator(
    op: Callable[..., None],
    a: sc.Variable,
    b: sc.Variable,
    data: npt.NDArray[Any],
) -> None:
    op(a, b)
    # Assume numpy operations are correct as comparator
    op(data, b.values)
    assert np.array_equal(a.values, data)


def test_binary_operations() -> None:
    _a = sc.Variable(dims=['x'], values=np.arange(1, 10, dtype=float))
    _b = sc.Variable(dims=['x'], values=np.arange(1, 10, dtype=float))
    a = _a['x', :]
    b = _b['x', :]

    data = np.copy(a.values)
    c = a + b
    assert type(c) is sc.Variable
    assert np.array_equal(c.values, data + data)
    c = a - b
    assert np.array_equal(c.values, data - data)
    c = a * b
    assert np.array_equal(c.values, data * data)
    c = a / b
    assert np.array_equal(c.values, data / data)

    apply_inplace_binary_operator(operator.iadd, a, b, data)
    apply_inplace_binary_operator(operator.isub, a, b, data)
    apply_inplace_binary_operator(operator.imul, a, b, data)
    apply_inplace_binary_operator(operator.itruediv, a, b, data)


def test_binary_float_operations() -> None:
    _a = sc.Variable(dims=['x'], values=np.arange(1, 10, dtype=float))
    a = _a['x', :]
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


def test_equal_not_equal() -> None:
    _a = sc.Variable(dims=['x'], values=np.arange(1, 10, dtype=float))
    _b = sc.Variable(dims=['x'], values=np.arange(1, 10, dtype=float))
    a = _a['x', :]
    b = _b['x', :]
    c = a + 2.0
    assert sc.identical(a, b)
    assert sc.identical(b, a)
    assert not sc.identical(a, c)
    assert not sc.identical(c, a)


def test_correct_temporaries() -> None:
    v = sc.Variable(dims=['x'], values=np.arange(100.0))
    b = sc.sqrt(v)['x', 0:10]
    assert len(b.values) == 10  # type: ignore[arg-type]
    b = b['x', 2:5]
    assert len(b.values) == 3  # type: ignore[arg-type]
