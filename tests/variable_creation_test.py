# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
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


def make_dummy(dims, shape, with_variances=False, **kwargs):
    # Not using empty to avoid a copy from uninitialized memory in `expected`.
    if with_variances:
        return sc.Variable(dims=dims,
                           values=np.full(shape, 63.0),
                           variances=np.full(shape, 12.0),
                           **kwargs)
    return sc.Variable(dims=dims, values=np.full(shape, 81.0), **kwargs)


def test_scalar_with_dtype():
    value = 1.0
    variance = 5.0
    unit = sc.units.m
    dtype = sc.dtype.float64
    var = sc.scalar(value=value, variance=variance, unit=unit, dtype=dtype)
    expected = sc.Variable(dims=(),
                           values=value,
                           variances=variance,
                           unit=unit,
                           dtype=dtype)
    assert sc.identical(var, expected)


def test_scalar_without_dtype():
    value = 'temp'
    var = sc.scalar(value)
    expected = sc.Variable(dims=(), values=value)
    assert sc.identical(var, expected)


def test_scalar_throws_if_wrong_dtype_provided_for_str_types():
    with pytest.raises(ValueError):
        sc.scalar(value='temp', unit=sc.units.one, dtype=sc.dtype.float64)


def test_scalar_throws_UnitError_if_not_parsable():
    with pytest.raises(sc.UnitError):
        sc.scalar(value=1, unit='abcdef')


def test_scalar_of_numpy_array():
    value = np.array([1, 2, 3])
    with pytest.raises(sc.DimensionError):
        sc.scalar(value)
    var = sc.scalar(value, dtype=sc.dtype.PyObject)
    assert var.dtype == sc.dtype.PyObject
    np.testing.assert_array_equal(var.value, value)


def test_zeros_creates_variable_with_correct_dims_and_shape():
    var = sc.zeros(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    expected = sc.Variable(dims=['x', 'y', 'z'], values=np.zeros([1, 2, 3]))
    assert sc.identical(var, expected)


def test_zeros_with_variances():
    shape = [1, 2, 3]
    var = sc.zeros(dims=['x', 'y', 'z'], shape=shape, with_variances=True)
    a = np.zeros(shape)
    expected = sc.Variable(dims=['x', 'y', 'z'], values=a, variances=a)
    assert sc.identical(var, expected)


def test_zeros_with_dtype_and_unit():
    var = sc.zeros(dims=['x', 'y', 'z'],
                   shape=[1, 2, 3],
                   dtype=sc.dtype.int32,
                   unit='m')
    assert var.dtype == sc.dtype.int32
    assert var.unit == 'm'


def test_zeros_dtypes():
    for dtype in (int, float, bool):
        assert sc.zeros(dims=(), shape=(), dtype=dtype).value == dtype(0)
    assert sc.zeros(dims=(), shape=(), unit='s',
                    dtype='datetime64').value == np.datetime64(0, 's')
    assert sc.zeros(dims=(), shape=(), dtype=str).value == ''
    np.testing.assert_array_equal(
        sc.zeros(dims=(), shape=(), dtype=sc.dtype.vector3).value, np.zeros(3))
    np.testing.assert_array_equal(
        sc.zeros(dims=(), shape=(), dtype=sc.dtype.linear_transform3).value,
        np.zeros((3, 3)))


def test_ones_creates_variable_with_correct_dims_and_shape():
    var = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    expected = sc.Variable(dims=['x', 'y', 'z'], values=np.ones([1, 2, 3]))
    assert sc.identical(var, expected)


def test_ones_with_variances():
    var = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3], with_variances=True)
    expected = sc.Variable(dims=['x', 'y', 'z'],
                           values=np.ones([1, 2, 3]),
                           variances=np.ones([1, 2, 3]))
    assert sc.identical(var, expected)


def test_ones_with_dtype_and_unit():
    var = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=sc.dtype.int64, unit='s')
    assert var.dtype == sc.dtype.int64
    assert var.unit == 's'


def test_ones_dtypes():
    for dtype in (int, float, bool):
        assert sc.ones(dims=(), shape=(), dtype=dtype).value == dtype(1)
    assert sc.ones(dims=(), shape=(), unit='s',
                   dtype='datetime64').value == np.datetime64(1, 's')
    with pytest.raises(ValueError):
        sc.ones(dims=(), shape=(), dtype=str)


def test_full_creates_variable_with_correct_dims_and_shape():
    var = sc.full(dims=['x', 'y', 'z'], shape=[1, 2, 3], value=12.34)
    expected = sc.Variable(dims=['x', 'y', 'z'], values=np.full([1, 2, 3], 12.34))
    assert sc.identical(var, expected)


def test_full_with_variances():
    var = sc.full(dims=['x', 'y', 'z'], shape=[1, 2, 3], value=12.34, variance=56.78)
    expected = sc.Variable(dims=['x', 'y', 'z'],
                           values=np.full([1, 2, 3], 12.34),
                           variances=np.full([1, 2, 3], 56.78))
    assert sc.identical(var, expected)


def test_full_with_dtype_and_unit():
    var = sc.full(dims=['x', 'y', 'z'],
                  shape=[1, 2, 3],
                  dtype=sc.dtype.int64,
                  unit='s',
                  value=1)
    assert var.dtype == sc.dtype.int64
    assert var.unit == 's'


def test_full_and_ones_equivalent():
    assert sc.identical(
        sc.full(dims=["x", "y"], shape=(2, 2), unit="m", value=1.0),
        sc.ones(dims=["x", "y"], shape=(2, 2), unit="m"),
    )


def test_full_and_zeros_equivalent():
    assert sc.identical(
        sc.full(dims=["x", "y"], shape=(2, 2), unit="m", value=0.0),
        sc.zeros(dims=["x", "y"], shape=(2, 2), unit="m"),
    )


def test_full_like():
    to_copy = sc.zeros(dims=["x", "y"], shape=(2, 2))

    assert sc.identical(sc.full_like(to_copy, value=123.45),
                        sc.full(dims=["x", "y"], shape=(2, 2), value=123.45))


def test_full_like_with_variance():
    to_copy = sc.zeros(dims=["x", "y"], shape=(2, 2))

    assert sc.identical(
        sc.full_like(to_copy, value=123.45, variance=67.89),
        sc.full(dims=["x", "y"], shape=(2, 2), value=123.45, variance=67.89))


def test_empty_creates_variable_with_correct_dims_and_shape():
    var = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    expected = make_dummy(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    _compare_properties(var, expected)


def test_empty_with_variances():
    var = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3], with_variances=True)
    expected = make_dummy(dims=['x', 'y', 'z'], shape=[1, 2, 3], with_variances=True)
    _compare_properties(var, expected)


def test_empty_with_dtype_and_unit():
    var = sc.empty(dims=['x', 'y', 'z'],
                   shape=[1, 2, 3],
                   dtype=sc.dtype.int32,
                   unit='s')
    assert var.dtype == sc.dtype.int32
    assert var.unit == 's'


def test_empty_dtypes():
    for dtype in (int, float, bool):
        var = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=dtype)
        expected = make_dummy(dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=dtype)
        _compare_properties(var, expected)
    var = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype='datetime64')
    expected = sc.Variable(dims=['x', 'y', 'z'],
                           values=np.full([1, 2, 3], 83),
                           dtype='datetime64')
    _compare_properties(var, expected)


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


def test_array_empty_dims():
    assert sc.identical(sc.array(dims=[], values=[1]),
                        sc.scalar([1], dtype=sc.dtype.PyObject))
    a = np.asarray(1.1)
    assert sc.identical(sc.array(dims=None, values=a), sc.scalar(1.1))
    assert sc.identical(sc.array(dims=[], values=a), sc.scalar(1.1))
    assert sc.identical(sc.array(dims=[], values=a, variances=a),
                        sc.scalar(1.1, variance=1.1))


def test_array_dim_from_keyword_fail_multiple():
    with pytest.raises(ValueError):
        sc.array(x=[4], y=[5])


def test_array_dim_from_keyword_fail_redundant_dims():
    with pytest.raises(ValueError):
        sc.array(dims=['x'], x=[4])
    with pytest.raises(ValueError):
        sc.array(dims=['y'], x=[4])


def test_array_dim_from_keyword_fail_redundant_values():
    with pytest.raises(ValueError):
        sc.array(x=[4], values=[4])


def test_array_dim_from_keyword():
    var = sc.array(xx=[2, 3, 4], unit='K', dtype='float32')
    assert sc.identical(
        var, sc.array(dims=['xx'], values=[2, 3, 4], unit='K', dtype='float32'))


def test_array_dim_from_keyword_with_variances():
    var = sc.array(xx=[2, 3, 4], unit='K', dtype='float32', variances=[3, 4, 5])
    assert sc.identical(
        var,
        sc.array(dims=['xx'],
                 values=[2, 3, 4],
                 variances=[3, 4, 5],
                 unit='K',
                 dtype='float32'))


def test_zeros_like():
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.zeros(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    zeros = sc.zeros_like(var)
    _compare_properties(zeros, expected)
    np.testing.assert_array_equal(zeros.values, 0)


def test_zeros_like_with_variances():
    var = sc.Variable(dims=['x', 'y', 'z'],
                      values=np.random.random([1, 2, 3]),
                      variances=np.random.random([1, 2, 3]),
                      unit='m',
                      dtype=sc.dtype.float32)
    expected = sc.zeros(dims=['x', 'y', 'z'],
                        shape=[1, 2, 3],
                        with_variances=True,
                        unit='m',
                        dtype=sc.dtype.float32)
    zeros = sc.zeros_like(var)
    _compare_properties(zeros, expected)
    np.testing.assert_array_equal(zeros.values, 0)
    np.testing.assert_array_equal(zeros.variances, 0)


def test_ones_like():
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    ones = sc.ones_like(var)
    _compare_properties(sc.ones_like(var), expected)
    np.testing.assert_array_equal(ones.values, 1)


def test_ones_like_with_variances():
    var = sc.Variable(dims=['x', 'y', 'z'],
                      values=np.random.random([1, 2, 3]),
                      variances=np.random.random([1, 2, 3]),
                      unit='m',
                      dtype=sc.dtype.float32)
    expected = sc.ones(dims=['x', 'y', 'z'],
                       shape=[1, 2, 3],
                       with_variances=True,
                       unit='m',
                       dtype=sc.dtype.float32)
    ones = sc.ones_like(var)
    _compare_properties(ones, expected)
    np.testing.assert_array_equal(ones.values, 1)
    np.testing.assert_array_equal(ones.variances, 1)


def test_empty_like():
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = make_dummy(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    _compare_properties(sc.empty_like(var), expected)


def test_empty_like_with_variances():
    var = sc.Variable(dims=['x', 'y', 'z'],
                      values=np.random.random([1, 2, 3]),
                      variances=np.random.random([1, 2, 3]),
                      unit='m',
                      dtype=sc.dtype.float32)
    expected = make_dummy(dims=['x', 'y', 'z'],
                          shape=[1, 2, 3],
                          with_variances=True,
                          unit='m',
                          dtype=sc.dtype.float32)
    _compare_properties(sc.empty_like(var), expected)


def test_linspace():
    values = np.linspace(1.2, 103., 51)
    var = sc.linspace('x', 1.2, 103., 51, unit='m', dtype=sc.dtype.float32)
    expected = sc.Variable(dims=['x'], values=values, unit='m', dtype=sc.dtype.float32)
    assert sc.identical(var, expected)


def test_logspace():
    values = np.logspace(2.0, 3.0, num=4)
    var = sc.logspace('y', 2.0, 3.0, num=4, unit='s')
    expected = sc.Variable(dims=['y'], values=values, unit='s', dtype=sc.dtype.float64)
    assert sc.identical(var, expected)


def test_geomspace():
    values = np.geomspace(1, 1000, num=4)
    var = sc.geomspace('z', 1, 1000, num=4)
    expected = sc.Variable(dims=['z'], values=values, dtype=sc.dtype.float64)
    assert sc.identical(var, expected)


def test_arange():
    values = np.arange(21)
    var = sc.arange('x', 21, unit='m', dtype=sc.dtype.int32)
    expected = sc.Variable(dims=['x'], values=values, unit='m', dtype=sc.dtype.int32)
    assert sc.identical(var, expected)
    values = np.arange(10, 21, 2)
    var = sc.arange(dim='x', start=10, stop=21, step=2, unit='m', dtype=sc.dtype.int32)
    expected = sc.Variable(dims=['x'], values=values, unit='m', dtype=sc.dtype.int32)
    assert sc.identical(var, expected)


def test_zeros_sizes():
    dims = ['x', 'y', 'z']
    shape = [2, 3, 4]
    assert sc.identical(sc.zeros(dims=dims, shape=shape),
                        sc.zeros(sizes=dict(zip(dims, shape))))
    with pytest.raises(ValueError):
        sc.zeros(dims=dims, shape=shape, sizes=dict(zip(dims, shape)))


def test_ones_sizes():
    dims = ['x', 'y', 'z']
    shape = [2, 3, 4]
    assert sc.identical(sc.ones(dims=dims, shape=shape),
                        sc.ones(sizes=dict(zip(dims, shape))))
    with pytest.raises(ValueError):
        sc.ones(dims=dims, shape=shape, sizes=dict(zip(dims, shape)))


def test_empty_sizes():
    dims = ['x', 'y', 'z']
    shape = [2, 3, 4]
    _compare_properties(sc.empty(dims=dims, shape=shape),
                        sc.empty(sizes=dict(zip(dims, shape))))
    with pytest.raises(ValueError):
        sc.empty(dims=dims, shape=shape, sizes=dict(zip(dims, shape)))
