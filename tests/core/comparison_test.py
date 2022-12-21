# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def test_comparison_operators():
    assert not sc.scalar(5) < sc.scalar(3)
    assert not sc.scalar(5) <= sc.scalar(3)
    assert sc.scalar(5) > sc.scalar(3)
    assert sc.scalar(5) >= sc.scalar(3)
    assert not sc.scalar(5) == sc.scalar(3)
    assert sc.scalar(5) != sc.scalar(3)


def test_comparison_functions():
    assert sc.less(sc.scalar(1), sc.scalar(2))
    assert sc.less_equal(sc.scalar(1), sc.scalar(2))
    assert not sc.greater(sc.scalar(1), sc.scalar(2))
    assert not sc.greater_equal(sc.scalar(1), sc.scalar(2))
    assert not sc.equal(sc.scalar(1), sc.scalar(2))
    assert sc.not_equal(sc.scalar(1), sc.scalar(2))


def test_isclose():
    unit = sc.units.m
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.identical(sc.isclose(a, a, rtol=0 * sc.units.one, atol=0 * unit),
                        sc.full(sizes={'x': 3}, value=True))


def test_isclose_atol_defaults():
    unit = sc.units.s
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.identical(sc.isclose(a, a, rtol=0 * sc.units.one),
                        sc.full(sizes={'x': 3}, value=True))


def test_isclose_rtol_defaults():
    unit = sc.units.kg
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.identical(sc.isclose(a, a, atol=0 * unit),
                        sc.full(sizes={'x': 3}, value=True))


def test_isclose_no_unit():
    a = sc.array(dims=['x'], values=[1, 2, 3], unit=None)
    assert sc.identical(sc.isclose(a, a), sc.full(sizes={'x': 3}, value=True))


def test_allclose():
    unit = sc.units.mm
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.allclose(a, a, rtol=0 * sc.units.one, atol=0 * unit)
    b = a.copy()
    b['x', 0] = 2
    assert not sc.allclose(a, b, rtol=0 * sc.units.one, atol=0 * unit)


def test_allclose_vectors():
    unit = sc.units.m
    a = sc.vectors(dims=['x'], values=[[1, 2, 3]], unit=unit)
    assert sc.allclose(a, a, atol=0 * unit)


def test_allclose_atol_defaults():
    unit = sc.units.m
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.allclose(a, a, rtol=0 * sc.units.one)


def test_allclose_rtol_defaults():
    unit = sc.units.s
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.allclose(a, a, atol=0 * unit)


def test_allclose_no_unit():
    a = sc.array(dims=['x'], values=[1, 2, 3], unit=None)
    assert sc.allclose(a, a)


@pytest.mark.parametrize('t',
                         (lambda x: x, sc.DataArray, lambda x: sc.Dataset({'a': x})))
def test_identical(t):
    assert sc.identical(t(sc.scalar(1.23)), t(sc.scalar(1.23)))
    assert not sc.identical(t(sc.scalar(1.23)), t(sc.scalar(1.23, unit='m')))
    assert not sc.identical(t(sc.scalar(1.23)), t(sc.scalar(5.2)))
    assert not sc.identical(t(sc.scalar(1.23)), t(sc.array(dims=['x'], values=[1.23])))
    assert sc.identical(t(sc.array(dims=['x'], values=[1.23], unit='m')),
                        t(sc.array(dims=['x'], values=[1.23], unit='m')))


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
