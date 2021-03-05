# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
import numpy as np
from .common import assert_export


def _check_comparison_ops_on(obj):
    assert_export(obj.__eq__)
    assert_export(obj.__ne__)
    assert_export(obj.__lt__)
    assert_export(obj.__gt__)
    assert_export(obj.__ge__)
    assert_export(obj.__le__)


def test_comparison_op_exports_for_variable():
    var = sc.Variable(['x'], values=np.array([1, 2, 3]))
    _check_comparison_ops_on(var)
    _check_comparison_ops_on(var['x', :])


def test_is_close():
    unit = sc.units.one
    a = sc.Variable(['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.all(sc.is_close(a, a, 0 * unit, 0 * unit)).value


def test_is_close_rtol_as_number():
    unit = sc.units.one
    a = sc.Variable(['x'], values=np.array([1, 2, 3]), unit=unit)
    # rtol as number (checks internal conversion to scipp scalar)
    assert sc.all(sc.is_close(a, a, 0, 0 * unit)).value


def test_is_close_atol_defaults():
    unit = sc.units.one
    a = sc.Variable(['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.all(sc.is_close(a, a, rtol=0)).value


def test_is_close_rtol_defaults():
    unit = sc.units.one
    a = sc.Variable(['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.all(sc.is_close(a, a, atol=0)).value


def test_is_equal():
    var = sc.Variable(['x'], values=np.array([1]))
    assert_export(sc.is_equal, var, var)
    assert_export(sc.is_equal, var['x', :], var['x', :])

    ds = sc.Dataset(data={'a': var})
    assert_export(sc.is_equal, ds['x', :], ds['x', :])
