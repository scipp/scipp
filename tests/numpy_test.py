# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def test_numpy_self_assign_shift() -> None:
    var = sc.Variable(dims=['x'], values=np.arange(8))
    expected = sc.Variable(dims=['x'], values=[0, 1, 0, 1, 2, 3, 6, 7])
    var['x', 2:6].values = var['x', 0:4].values
    assert sc.identical(var, expected)


def test_numpy_self_assign_1d_flip() -> None:
    var = sc.Variable(dims=['x'], values=np.arange(100))
    expected = sc.Variable(dims=['x'], values=np.flip(var.values))
    var.values = np.flip(var.values)
    assert sc.identical(var, expected)


def test_numpy_self_assign_2d_flip_both() -> None:
    var = sc.Variable(dims=['y', 'x'], values=np.arange(100).reshape(10, 10))
    expected = sc.Variable(dims=['y', 'x'], values=np.flip(var.values))
    var.values = np.flip(var.values)
    assert sc.identical(var, expected)


def test_numpy_self_assign_2d_flip_first() -> None:
    var = sc.Variable(dims=['y', 'x'], values=np.arange(100).reshape(10, 10))
    expected = sc.Variable(dims=['y', 'x'], values=np.flip(var.values, axis=0))
    var.values = np.flip(var.values, axis=0)
    assert sc.identical(var, expected)


def test_numpy_self_assign_2d_flip_second() -> None:
    var = sc.Variable(dims=['y', 'x'], values=np.arange(100).reshape(10, 10))
    expected = sc.Variable(dims=['y', 'x'], values=np.flip(var.values, axis=1))
    var.values = np.flip(var.values, axis=1)
    assert sc.identical(var, expected)


def test_numpy_self_assign_shift_2d_flip_both() -> None:
    var = sc.Variable(dims=['y', 'x'], values=np.arange(9).reshape(3, 3))
    expected = sc.Variable(
        dims=['y', 'x'], values=np.array([[0, 1, 2], [3, 4, 3], [6, 1, 0]])
    )
    var['y', 1:3]['x', 1:3].values = np.flip(var['y', 0:2]['x', 0:2].values)
    assert sc.identical(var, expected)


def test_numpy_self_assign_shift_2d_flip_first() -> None:
    # Special case of shift combined with negative stride: Essentially we walk
    # backwards from "begin", but away from "end" since the outer dim has a
    # positive stride, so a naive range check based does not catch this case.
    var = sc.Variable(dims=['y', 'x'], values=np.arange(9).reshape(3, 3))
    expected = sc.Variable(
        dims=['y', 'x'], values=np.array([[0, 1, 2], [3, 3, 4], [6, 0, 1]])
    )
    var['y', 1:3]['x', 1:3].values = np.flip(var['y', 0:2]['x', 0:2].values, axis=0)
    assert sc.identical(var, expected)


def test_numpy_self_assign_shift_2d_flip_second() -> None:
    var = sc.Variable(dims=['y', 'x'], values=np.arange(9).reshape(3, 3))
    expected = sc.Variable(
        dims=['y', 'x'], values=np.array([[0, 1, 2], [3, 1, 0], [6, 4, 3]])
    )
    var['y', 1:3]['x', 1:3].values = np.flip(var['y', 0:2]['x', 0:2].values, axis=1)
    assert sc.identical(var, expected)


a = np.arange(2)
var = sc.Variable(dims=['x'], values=a)
arr = sc.DataArray(var)
ds = sc.Dataset(data={'a': arr})


@pytest.mark.parametrize("obj", [var, var['x', :], arr, arr['x', :], ds, ds['x', :]])
def test__array_ufunc___disabled(obj: sc.Variable | sc.DataArray | sc.Dataset) -> None:
    with pytest.raises(TypeError, match="does not support ufuncs"):
        obj + a  # type: ignore[operator]
    with pytest.raises(TypeError, match="unsupported operand type"):
        a * obj  # type: ignore[operator]
    with pytest.raises(TypeError, match="does not support ufuncs"):
        obj += a  # type: ignore[operator]

    b = np.arange(2)
    with pytest.raises(TypeError, match="does not support ufuncs"):
        b += obj  # type: ignore[arg-type]
