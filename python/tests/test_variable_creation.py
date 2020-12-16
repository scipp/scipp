# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc


def test_scalar_with_dtype():
    value = 1.0
    variance = 5.0
    unit = sc.units.m
    dtype = sc.dtype.float64
    var = sc.scalar(value=value, variance=variance, unit=unit, dtype=dtype)
    expected = sc.Variable(value=value,
                           variance=variance,
                           unit=unit,
                           dtype=dtype)

    comparison = var == expected
    assert comparison.values.all()


def test_scalar_without_dtype():
    value = 'temp'
    var = sc.scalar(value)
    expected = sc.Variable(value)

    # Cannot directly compare variables with string dtype
    assert var.values == expected.values


def test_scalar_throws_if_dtype_provided_for_str_types():
    with pytest.raises(TypeError):
        sc.scalar(value='temp', unit=sc.units.one, dtype=sc.dtype.float64)


def test_zeros_creates_variable_with_correct_dims_and_shape():
    var = sc.zeros(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    expected = sc.Variable(dims=['x', 'y', 'z'], shape=[1, 2, 3])

    comparison = var == expected
    assert comparison.values.all()


def test_array_creates_correct_variable():
    dims = ['x']
    values = [1, 2, 3]
    variances = [4, 5, 6]
    unit = sc.units.m
    dtype = sc.dtype.float64
    var = sc.array(dims=dims,
                   values=values,
                   variances=variances,
                   unit=unit,
                   dtype=dtype)
    expected = sc.Variable(dims=dims,
                           values=values,
                           variances=variances,
                           unit=unit,
                           dtype=dtype)

    comparison = var == expected
    assert comparison.values.all()
