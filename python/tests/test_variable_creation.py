# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc
import numpy as np


def _compare_properties(a, b):
    assert a.dims == b.dims
    assert a.shape == b.shape
    assert a.unit == b.unit
    assert a.dtype == b.dtype
    assert (a.variances is None) == (b.variances is None)


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
    assert sc.identical(var, expected)


def test_scalar_without_dtype():
    value = 'temp'
    var = sc.scalar(value)
    expected = sc.Variable(value)
    assert sc.identical(var, expected)


def test_scalar_throws_if_dtype_provided_for_str_types():
    with pytest.raises(TypeError):
        sc.scalar(value='temp', unit=sc.units.one, dtype=sc.dtype.float64)


def test_zeros_creates_variable_with_correct_dims_and_shape():
    var = sc.zeros(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    expected = sc.Variable(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    assert sc.identical(var, expected)


def test_zeros_with_variances():
    var = sc.zeros(dims=['x', 'y', 'z'], shape=[1, 2, 3], variances=True)
    expected = sc.Variable(dims=['x', 'y', 'z'],
                           shape=[1, 2, 3],
                           variances=True)
    assert sc.identical(var, expected)


def test_zeros_with_dtype_and_unit():
    var = sc.zeros(dims=['x', 'y', 'z'],
                   shape=[1, 2, 3],
                   dtype=sc.dtype.int32,
                   unit='m')
    assert var.dtype == sc.dtype.int32
    assert var.unit == 'm'


def test_ones_creates_variable_with_correct_dims_and_shape():
    var = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    expected = sc.Variable(dims=['x', 'y', 'z'], values=np.ones([1, 2, 3]))
    assert sc.identical(var, expected)


def test_ones_with_variances():
    var = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3], variances=True)
    expected = sc.Variable(dims=['x', 'y', 'z'],
                           values=np.ones([1, 2, 3]),
                           variances=np.ones([1, 2, 3]))
    assert sc.identical(var, expected)


def test_ones_with_dtype_and_unit():
    var = sc.ones(dims=['x', 'y', 'z'],
                  shape=[1, 2, 3],
                  dtype=sc.dtype.int64,
                  unit='s')
    assert var.dtype == sc.dtype.int64
    assert var.unit == 's'


def test_empty_creates_variable_with_correct_dims_and_shape():
    var = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    expected = sc.Variable(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    _compare_properties(var, expected)


def test_empty_with_variances():
    var = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3], variances=True)
    expected = sc.Variable(dims=['x', 'y', 'z'],
                           shape=[1, 2, 3],
                           variances=True)
    _compare_properties(var, expected)


def test_empty_with_dtype_and_unit():
    var = sc.empty(dims=['x', 'y', 'z'],
                   shape=[1, 2, 3],
                   dtype=sc.dtype.int32,
                   unit='s')
    assert var.dtype == sc.dtype.int32
    assert var.unit == 's'


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

    assert sc.identical(var, expected)


def test_zeros_like():
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.zeros(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    _compare_properties(sc.zeros_like(var), expected)


def test_zeros_like_with_variances():
    var = sc.Variable(dims=['x', 'y', 'z'],
                      values=np.random.random([1, 2, 3]),
                      variances=np.random.random([1, 2, 3]),
                      unit='m',
                      dtype=sc.dtype.float32)
    expected = sc.zeros(dims=['x', 'y', 'z'],
                        shape=[1, 2, 3],
                        variances=True,
                        unit='m',
                        dtype=sc.dtype.float32)
    _compare_properties(sc.zeros_like(var), expected)


def test_ones_like():
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    _compare_properties(sc.ones_like(var), expected)


def test_ones_like_with_variances():
    var = sc.Variable(dims=['x', 'y', 'z'],
                      values=np.random.random([1, 2, 3]),
                      variances=np.random.random([1, 2, 3]),
                      unit='m',
                      dtype=sc.dtype.float32)
    expected = sc.ones(dims=['x', 'y', 'z'],
                       shape=[1, 2, 3],
                       variances=True,
                       unit='m',
                       dtype=sc.dtype.float32)
    _compare_properties(sc.ones_like(var), expected)


def test_empty_like():
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.Variable(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    _compare_properties(sc.empty_like(var), expected)


def test_empty_like_with_variances():
    var = sc.Variable(dims=['x', 'y', 'z'],
                      values=np.random.random([1, 2, 3]),
                      variances=np.random.random([1, 2, 3]),
                      unit='m',
                      dtype=sc.dtype.float32)
    expected = sc.Variable(dims=['x', 'y', 'z'],
                           shape=[1, 2, 3],
                           variances=True,
                           unit='m',
                           dtype=sc.dtype.float32)
    _compare_properties(sc.empty_like(var), expected)
