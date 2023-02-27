# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from copy import deepcopy

import numpy as np
import pytest

import scipp as sc
from scipp.testing import assert_identical


@pytest.mark.parametrize('a', (3, -1.2, 'hjh wed', [], {4}))
def test_assert_identical_builtin_true(a):
    assert_identical(deepcopy(a), deepcopy(a))


@pytest.mark.parametrize('a', (3, -1.2, 'hjh wed', [], {4}))
@pytest.mark.parametrize('b', (1, 0.2, 'll', [7], {}))
def test_assert_identical_builtin_false(a, b):
    with pytest.raises(AssertionError):
        assert_identical(a, b)


@pytest.mark.parametrize(
    'a', (sc.scalar(3), sc.scalar(7.12, variance=0.33, unit='m'),
          sc.arange('u', 9.5, 13.0, 0.4), sc.linspace('ppl', 3.7, -99, 10, unit='kg'),
          sc.array(dims=['ww', 'gas'], values=[[np.nan], [3]])))
def test_assert_identical_variables_true(a):
    assert_identical(deepcopy(a), deepcopy(a))


def test_assert_identical_variables_dim_mismatch():
    a = sc.arange('rst', 5, unit='m')
    b = sc.arange('llf', 5, unit='m')
    with pytest.raises(AssertionError):
        assert_identical(a, b)
    with pytest.raises(AssertionError):
        assert_identical(b, a)

    a = sc.arange('t', 12, unit='m').fold('t', {'x': 3, 'k': 4})
    b = sc.arange('t', 12, unit='m').fold('t', {'x': 3, 't': 4})
    with pytest.raises(AssertionError):
        assert_identical(a, b)
    with pytest.raises(AssertionError):
        assert_identical(b, a)


def test_assert_identical_variables_shape_mismatch():
    a = sc.arange('x', 5, unit='m')
    b = sc.arange('x', 6, unit='m')
    with pytest.raises(AssertionError):
        assert_identical(a, b)
    with pytest.raises(AssertionError):
        assert_identical(b, a)

    a = sc.arange('i', 5, unit='m')
    b = sc.arange('t', 10, unit='m').fold('t', {'x': 5, 'k': 2})
    with pytest.raises(AssertionError):
        assert_identical(a, b)
    with pytest.raises(AssertionError):
        assert_identical(b, a)


def test_assert_identical_unit_mismatch():
    a = sc.arange('u', 6.1, unit='m')
    b = sc.arange('u', 6.1, unit='kg')
    with pytest.raises(AssertionError):
        assert_identical(a, b)
    with pytest.raises(AssertionError):
        assert_identical(b, a)


def test_assert_identical_values_mismatch():
    a = sc.arange('u', 6.1, unit='m')
    b = sc.arange('u', 6.1, unit='m') * 0.1
    with pytest.raises(AssertionError):
        assert_identical(a, b)
    with pytest.raises(AssertionError):
        assert_identical(b, a)


def test_assert_identical_values_mismatch_nan():
    a = sc.arange('u', 6.1, unit='m')
    b = sc.arange('u', 6.1, unit='m')
    b[2] = np.nan
    with pytest.raises(AssertionError):
        assert_identical(a, b)
    with pytest.raises(AssertionError):
        assert_identical(b, a)


def test_assert_identical_variances_mismatch():
    a = sc.arange('u', 6.1, unit='m')
    a.variances = a.values
    b = sc.arange('u', 6.1, unit='m')
    b.variances = b.values * 1.2
    with pytest.raises(AssertionError):
        assert_identical(a, b)
    with pytest.raises(AssertionError):
        assert_identical(b, a)


def test_assert_identical_variances_mismatch_nan():
    a = sc.arange('u', 6.1, unit='m')
    a.variances = a.values
    b = sc.arange('u', 6.1, unit='m')
    b.variances = b.values
    b.variances[1] = np.nan
    with pytest.raises(AssertionError):
        assert_identical(a, b)
    with pytest.raises(AssertionError):
        assert_identical(b, a)


def test_assert_identical_presence_of_variances():
    a = sc.scalar(1.1, variance=0.1)
    b = sc.scalar(1.1)
    with pytest.raises(AssertionError):
        assert_identical(a, b)
    with pytest.raises(AssertionError):
        assert_identical(b, a)
