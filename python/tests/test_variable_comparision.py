# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
import numpy as np
from .common import assert_export
import pytest


def _check_comparison_ops_on(obj):
    assert_export(obj.__eq__)
    assert_export(obj.__ne__)
    assert_export(obj.__lt__)
    assert_export(obj.__gt__)
    assert_export(obj.__ge__)
    assert_export(obj.__le__)


def test_comparison_op_exports():
    var = sc.Variable(['x'], values=np.array([1, 2, 3]))
    _check_comparison_ops_on(var)
    _check_comparison_ops_on(var['x', :])


def test_is_approx_int():
    # non-trivial bindings
    a = sc.Variable(['x'], values=np.array([1, 2, 3]))
    b = sc.Variable(['x'], values=np.array([2, 2, 3]))
    assert sc.is_approx(a, a, 0)
    assert sc.is_approx(a, b, 1)
    assert sc.is_approx(a['x', :], b, 1)
    assert sc.is_approx(a, b['x', :], 1)
    assert sc.is_approx(a['x', :], b['x', :], 1)


def test_is_approx_float():
    # non-trivial bindings
    a = sc.Variable(['x'], values=np.array([1, 2, 3]), dtype=sc.dtype.float64)
    b = sc.Variable(['x'], values=np.array([2, 2, 3]), dtype=sc.dtype.float64)
    assert sc.is_approx(a, a, 0.0)
    assert sc.is_approx(a, b, 1.0)
    assert sc.is_approx(a['x', :], b, 1.0)
    assert sc.is_approx(a, b['x', :], 1.0)
    assert sc.is_approx(a['x', :], b['x', :], 1.0)


def test_tol_type_mismatch():
    a = sc.Variable(['x'], values=np.array([1, 2, 3]), dtype=sc.dtype.int64)
    with pytest.raises(RuntimeError):
        # tol is float
        sc.is_approx(a, a, 1.0)


def test_is_approx_unsupported_type():
    a = sc.Variable(value=sc.Dataset())
    with pytest.raises(RuntimeError):
        sc.is_approx(a, a, 1)


def test_is_equal():
    var = sc.Variable(['x'], values=np.array([1]))
    assert_export(sc.is_equal, var, var)
