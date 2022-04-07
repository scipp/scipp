# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest
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
    var = sc.Variable(dims=['x'], values=np.array([1, 2, 3]))
    _check_comparison_ops_on(var)
    _check_comparison_ops_on(var['x', :])


def test_isclose():
    unit = sc.units.one
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.all(sc.isclose(a, a, rtol=0 * unit, atol=0 * unit)).value


def test_isclose_atol_defaults():
    unit = sc.units.one
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.all(sc.isclose(a, a, rtol=0 * unit)).value


def test_isclose_rtol_defaults():
    unit = sc.units.one
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.all(sc.isclose(a, a, atol=0 * unit)).value


def test_isclose_no_unit():
    a = sc.array(dims=['x'], values=[1, 2, 3], unit=None)
    assert sc.identical(sc.isclose(a, a), sc.full(sizes={'x': 3}, value=True))


def test_allclose():
    unit = sc.units.one
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.allclose(a, a, 0 * unit, 0 * unit)
    b = a.copy()
    b['x', 0] = 2
    assert not sc.allclose(a, b, 0 * unit, 0 * unit)


def test_allclose_vectors():
    unit = sc.units.m
    a = sc.vectors(dims=['x'], values=[[1, 2, 3]], unit=unit)
    assert sc.allclose(a, a, atol=0 * unit)


def test_allclose_atol_defaults():
    unit = sc.units.one
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.allclose(a, a, rtol=0 * unit)


def test_allclose_rtol_defaults():
    unit = sc.units.one
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.allclose(a, a, atol=0 * unit)


def test_identical():
    var = sc.Variable(dims=['x'], values=np.array([1]))
    assert_export(sc.identical, var, var)
    assert_export(sc.identical, var['x', :], var['x', :])

    ds = sc.Dataset(data={'a': var})
    assert_export(sc.identical, ds['x', :], ds['x', :])


@pytest.fixture
def small():
    return sc.scalar(1.0)


@pytest.fixture
def medium():
    return sc.scalar(2.0)


@pytest.fixture
def large():
    return sc.scalar(3.0)


def test_eq(small, medium, large):
    assert not (small == medium).value
    assert not (small == medium.value).value
    assert not (small.values == medium).value

    assert (medium == medium).value
    assert (medium == medium.value).value
    assert (medium.value == medium).value

    assert not (large == medium).value
    assert not (large == medium.value).value
    assert not (large.value == medium).value


def test_ne(small, medium, large):
    assert (small != medium).value
    assert (small != medium.value).value
    assert (small.values != medium).value

    assert not (medium != medium).value
    assert not (medium != medium.value).value
    assert not (medium.value != medium).value

    assert (large != medium).value
    assert (large != medium.value).value
    assert (large.value != medium).value


def test_lt(small, medium, large):
    assert (small < medium).value
    assert (small < medium.value).value
    assert (small.values < medium).value

    assert not (medium < medium).value
    assert not (medium < medium.value).value
    assert not (medium.value < medium).value

    assert not (large < medium).value
    assert not (large < medium.value).value
    assert not (large.value < medium).value


def test_le(small, medium, large):
    assert (small <= medium).value
    assert (small <= medium.value).value
    assert (small.values <= medium).value

    assert (medium <= medium).value
    assert (medium <= medium.value).value
    assert (medium.value <= medium).value

    assert not (large <= medium).value
    assert not (large <= medium.value).value
    assert not (large.value <= medium).value


def test_gt(small, medium, large):
    assert not (small > medium).value
    assert not (small > medium.value).value
    assert not (small.values > medium).value

    assert not (medium > medium).value
    assert not (medium > medium.value).value
    assert not (medium.value > medium).value

    assert (large > medium).value
    assert (large > medium.value).value
    assert (large.value > medium).value


def test_ge(small, medium, large):
    assert not (small >= medium).value
    assert not (small >= medium.value).value
    assert not (small.values >= medium).value

    assert (medium >= medium).value
    assert (medium >= medium.value).value
    assert (medium.value >= medium).value

    assert (large >= medium).value
    assert (large >= medium.value).value
    assert (large.value >= medium).value


def test_comparison_with_str(medium):
    assert (sc.scalar('a string') == sc.scalar('a string')).value
    assert not (medium == 'a string')
    with pytest.raises(TypeError):
        _ = medium < 'a string'
