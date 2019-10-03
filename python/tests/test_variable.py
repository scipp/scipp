# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import scipp as sp
from scipp import Dim
import numpy as np


def make_variables():
    data = np.arange(1, 4, dtype=float)
    a = sp.Variable([sp.Dim.X], data)
    b = sp.Variable([sp.Dim.X], data)
    a_slice = a[sp.Dim.X, :]
    b_slice = b[sp.Dim.X, :]
    return a, b, a_slice, b_slice, data


def test_create_default():
    var = sp.Variable()
    assert var.dims == []
    assert var.sparse_dim is None
    assert var.dtype == sp.dtype.double
    assert var.unit == sp.units.dimensionless
    assert var.value == 0.0


def test_create_default_dtype():
    var = sp.Variable([sp.Dim.X], [4])
    assert var.dtype == sp.dtype.double


def test_create_with_dtype():
    var = sp.Variable(dims=[Dim.X], shape=[2], dtype=sp.dtype.float)
    assert var.dtype == sp.dtype.float


def test_create_with_numpy_dtype():
    var = sp.Variable(dims=[Dim.X], shape=[2], dtype=np.dtype(np.float32))
    assert var.dtype == sp.dtype.float


def test_create_with_variances():
    assert sp.Variable(dims=[Dim.X], shape=[2]).variances is None
    assert sp.Variable(dims=[Dim.X], shape=[2],
                       variances=False).variances is None
    assert sp.Variable(dims=[Dim.X], shape=[2],
                       variances=True).variances is not None


def test_create_with_shape_and_variances():
    # If no values are given, variances must be Bool, cannot pass array.
    with pytest.raises(TypeError):
        sp.Variable(dims=[Dim.X], shape=[2], variances=np.arange(2))


def test_create_sparse():
    var = sp.Variable([sp.Dim.X, sp.Dim.Y], [4, sp.Dimensions.Sparse])
    assert var.dtype == sp.dtype.double
    assert var.sparse_dim == sp.Dim.Y
    assert len(var.values) == 4
    for vals in var.values:
        assert len(vals) == 0


def test_create_from_numpy_1d():
    var = sp.Variable([sp.Dim.X], np.arange(4.0))
    assert var.dtype == sp.dtype.double
    np.testing.assert_array_equal(var.values, np.arange(4))


def test_create_from_numpy_1d_bool():
    var = sp.Variable(dims=[sp.Dim.X], values=np.array([True, False, True]))
    assert var.dtype == sp.dtype.bool
    np.testing.assert_array_equal(var.values, np.array([True, False, True]))


def test_create_with_variances_from_numpy_1d():
    var = sp.Variable([sp.Dim.X],
                      values=np.arange(4.0),
                      variances=np.arange(4.0, 8.0))
    assert var.dtype == sp.dtype.double
    np.testing.assert_array_equal(var.values, np.arange(4))
    np.testing.assert_array_equal(var.variances, np.arange(4, 8))


def test_create_scalar():
    var = sp.Variable(1.2)
    assert var.value == 1.2
    assert var.dims == []
    assert var.dtype == sp.dtype.double
    assert var.unit == sp.units.dimensionless


def test_create_scalar_Dataset():
    dataset = sp.Dataset({'a': sp.Variable([sp.Dim.X], np.arange(4.0))})
    var = sp.Variable(dataset)
    assert var.value == dataset
    assert var.dims == []
    assert var.dtype == sp.dtype.Dataset
    assert var.unit == sp.units.dimensionless


def test_create_scalar_quantity():
    var = sp.Variable(1.2, unit=sp.units.m)
    assert var.value == 1.2
    assert var.dims == []
    assert var.dtype == sp.dtype.double
    assert var.unit == sp.units.m


def test_create_via_unit():
    expected = sp.Variable(1.2, unit=sp.units.m)
    var = 1.2 * sp.units.m
    assert var == expected


def test_create_1D_string():
    var = sp.Variable(dims=[Dim.Row], values=['a', 'bb'], unit=sp.units.m)
    assert len(var.values) == 2
    assert var.values[0] == 'a'
    assert var.values[1] == 'bb'
    assert var.dims == [Dim.Row]
    assert var.dtype == sp.dtype.string
    assert var.unit == sp.units.m


def test_create_1D_vector_3_double():
    var = sp.Variable(dims=[Dim.X],
                      values=[[1, 2, 3], [4, 5, 6]],
                      unit=sp.units.m)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == [Dim.X]
    assert var.dtype == sp.dtype.vector_3_double
    assert var.unit == sp.units.m


def test_create_2D_inner_size_3():
    var = sp.Variable(dims=[Dim.X, Dim.Y],
                      values=np.arange(6.0).reshape(2, 3),
                      unit=sp.units.m)
    assert var.shape == [2, 3]
    np.testing.assert_array_equal(var.values[0], [0, 1, 2])
    np.testing.assert_array_equal(var.values[1], [3, 4, 5])
    assert var.dims == [Dim.X, Dim.Y]
    assert var.dtype == sp.dtype.double
    assert var.unit == sp.units.m


def test_operation_with_scalar_quantity():
    reference = sp.Variable([sp.Dim.X], np.arange(4.0) * 1.5)
    reference.unit = sp.units.kg

    var = sp.Variable([sp.Dim.X], np.arange(4.0))
    var *= sp.Variable(1.5, unit=sp.units.kg)
    assert var == reference


def test_0D_scalar_access():
    var = sp.Variable()
    assert var.value == 0.0
    var.value = 1.2
    assert var.value == 1.2
    assert var.values.shape == ()
    assert var.values == 1.2


def test_0D_scalar_string():
    var = sp.Variable(value='a')
    assert var.value == 'a'
    var.value = 'b'
    assert var == sp.Variable(value='b')


def test_1D_scalar_access_fail():
    var = sp.Variable([Dim.X], (1, ))
    with pytest.raises(RuntimeError):
        assert var.value == 0.0
    with pytest.raises(RuntimeError):
        var.value = 1.2


def test_1D_access():
    var = sp.Variable([Dim.X], (2, ))
    assert len(var.values) == 2
    assert var.values.shape == (2, )
    var.values[1] = 1.2
    assert var.values[1] == 1.2


def test_1D_set_from_list():
    var = sp.Variable([Dim.X], (2, ))
    var.values = [1.0, 2.0]
    assert var == sp.Variable([Dim.X], values=[1.0, 2.0])


def test_1D_string():
    var = sp.Variable([Dim.X], values=['a', 'b'])
    assert len(var.values) == 2
    assert var.values[0] == 'a'
    assert var.values[1] == 'b'
    var.values = ['c', 'd']
    assert var == sp.Variable([Dim.X], values=['c', 'd'])


def test_1D_converting():
    var = sp.Variable([Dim.X], values=[1, 2])
    var.values = [3.3, 4.6]
    # floats get truncated
    assert var == sp.Variable([Dim.X], values=[3, 4])


def test_1D_dataset():
    var = sp.Variable([Dim.X], shape=(2, ), dtype=sp.dtype.Dataset)
    d1 = sp.Dataset({'a': 1.5 * sp.units.m})
    d2 = sp.Dataset({'a': 2.5 * sp.units.m})
    var.values = [d1, d2]
    assert var.values[0] == d1
    assert var.values[1] == d2


def test_1D_access_bad_shape_fail():
    var = sp.Variable([Dim.X], (2, ))
    with pytest.raises(RuntimeError):
        var.values = np.arange(3)


def test_2D_access():
    var = sp.Variable([Dim.X, Dim.Y], (2, 3))
    assert var.values.shape == (2, 3)
    assert len(var.values) == 2
    assert len(var.values[0]) == 3
    var.values[1] = 1.2  # numpy assigns to all elements in "slice"
    var.values[1][2] = 2.2
    assert var.values[1][0] == 1.2
    assert var.values[1][1] == 1.2
    assert var.values[1][2] == 2.2


def test_2D_access_bad_shape_fail():
    var = sp.Variable([Dim.X, Dim.Y], (2, 3))
    with pytest.raises(RuntimeError):
        var.values = np.ones(shape=(3, 2))


def test_2D_access_variances():
    var = sp.Variable([Dim.X, Dim.Y], (2, 3), variances=True)
    assert var.values.shape == (2, 3)
    assert var.variances.shape == (2, 3)
    var.values[1] = 1.2
    assert np.array_equal(var.variances, np.zeros(shape=(2, 3)))
    var.variances = np.ones(shape=(2, 3))
    assert np.array_equal(var.variances, np.ones(shape=(2, 3)))


def test_sparse_slice():
    var = sp.Variable([sp.Dim.X, sp.Dim.Y], [4, sp.Dimensions.Sparse])
    vals0 = var[Dim.X, 0].values
    assert len(vals0) == 0
    vals0.append(1.2)
    assert len(var[Dim.X, 0].values) == 1


def test_sparse_setitem():
    var = sp.Variable([sp.Dim.X, sp.Dim.Y], [4, sp.Dimensions.Sparse])
    var[Dim.X, 0].values = np.arange(4)
    assert len(var[Dim.X, 0].values) == 4


def test_sparse_setitem_sparse_fail():
    var = sp.Variable([sp.Dim.X, sp.Dim.Y], [4, sp.Dimensions.Sparse])
    with pytest.raises(RuntimeError):
        var.values = np.arange(3)


def test_sparse_setitem_shape_fail():
    var = sp.Variable([sp.Dim.X, sp.Dim.Y], [4, sp.Dimensions.Sparse])
    with pytest.raises(RuntimeError):
        var[Dim.X, 0].values = np.ones(shape=(3, 2))


def test_sparse_setitem_float():
    var = sp.Variable([sp.Dim.X, sp.Dim.Y], [4, sp.Dimensions.Sparse],
                      dtype=sp.dtype.float)
    var[Dim.X, 0].values = np.arange(4)
    assert len(var[Dim.X, 0].values) == 4


def test_sparse_setitem_int64_t():
    var = sp.Variable([sp.Dim.X, sp.Dim.Y], [4, sp.Dimensions.Sparse],
                      dtype=sp.dtype.int64)
    var[Dim.X, 0].values = np.arange(4)
    assert len(var[Dim.X, 0].values) == 4


def test_create_dtype():
    var = sp.Variable([Dim.X], values=np.arange(4))
    assert var.dtype == sp.dtype.int64
    var = sp.Variable([Dim.X], values=np.arange(4).astype(np.int32))
    assert var.dtype == sp.dtype.int32
    var = sp.Variable([Dim.X], values=np.arange(4).astype(np.float64))
    assert var.dtype == sp.dtype.double
    var = sp.Variable([Dim.X], values=np.arange(4).astype(np.float32))
    assert var.dtype == sp.dtype.float
    var = sp.Variable([Dim.X], (4, ), dtype=np.dtype(np.float64))
    assert var.dtype == sp.dtype.double
    var = sp.Variable([Dim.X], (4, ), dtype=np.dtype(np.float32))
    assert var.dtype == sp.dtype.float
    var = sp.Variable([Dim.X], (4, ), dtype=np.dtype(np.int64))
    assert var.dtype == sp.dtype.int64
    var = sp.Variable([Dim.X], (4, ), dtype=np.dtype(np.int32))
    assert var.dtype == sp.dtype.int32


def test_get_slice():
    var = sp.Variable([Dim.X, Dim.Y], values=np.arange(0, 8).reshape(2, 4))
    var_slice = var[Dim.X, 1:2]
    assert var_slice == sp.Variable([Dim.X, Dim.Y],
                                    values=np.arange(4, 8).reshape(1, 4))


def test_slicing():
    var = sp.Variable([sp.Dim.X], values=np.arange(0, 3))
    var_slice = var[(sp.Dim.X, slice(0, 2))]
    assert isinstance(var_slice, sp.VariableProxy)
    assert len(var_slice.values) == 2
    assert np.array_equal(var_slice.values, np.array([0, 1]))


def test_binary_plus():
    a, b, a_slice, b_slice, data = make_variables()
    c = a + b
    assert np.array_equal(c.values, data + data)
    c = a + 2.0
    assert np.array_equal(c.values, data + 2.0)
    c = a + b_slice
    assert np.array_equal(c.values, data + data)
    c += b
    assert np.array_equal(c.values, data + data + data)
    c += b_slice
    assert np.array_equal(c.values, data + data + data + data)
    c = 3.5 + c
    assert np.array_equal(c.values, data + data + data + data + 3.5)


def test_binary_minus():
    a, b, a_slice, b_slice, data = make_variables()
    c = a - b
    assert np.array_equal(c.values, data - data)
    c = a - 2.0
    assert np.array_equal(c.values, data - 2.0)
    c = a - b_slice
    assert np.array_equal(c.values, data - data)
    c -= b
    assert np.array_equal(c.values, data - data - data)
    c -= b_slice
    assert np.array_equal(c.values, data - data - data - data)
    c = 3.5 - c
    assert np.array_equal(c.values, 3.5 - data + data + data + data)


def test_binary_multiply():
    a, b, a_slice, b_slice, data = make_variables()
    c = a * b
    assert np.array_equal(c.values, data * data)
    c = a * 2.0
    assert np.array_equal(c.values, data * 2.0)
    c = a * b_slice
    assert np.array_equal(c.values, data * data)
    c *= b
    assert np.array_equal(c.values, data * data * data)
    c *= b_slice
    assert np.array_equal(c.values, data * data * data * data)
    c = 3.5 * c
    assert np.array_equal(c.values, data * data * data * data * 3.5)


def test_binary_divide():
    a, b, a_slice, b_slice, data = make_variables()
    c = a / b
    assert np.array_equal(c.values, data / data)
    c = a / 2.0
    assert np.array_equal(c.values, data / 2.0)
    c = a / b_slice
    assert np.array_equal(c.values, data / data)
    c /= b
    assert np.array_equal(c.values, data / data / data)
    c /= b_slice
    assert np.array_equal(c.values, data / data / data / data)


def test_binary_equal():
    a, b, a_slice, b_slice, data = make_variables()
    assert a == b
    assert a == a_slice
    assert a_slice == b_slice
    assert b == a
    assert b_slice == a
    assert b_slice == a_slice


def test_binary_not_equal():
    a, b, a_slice, b_slice, data = make_variables()
    c = a + b
    assert a != c
    assert a_slice != c
    assert c != a
    assert c != a_slice


def test_abs():
    var = sp.Variable([Dim.X], values=np.array([0.1, -0.2]), unit=sp.units.m)
    expected = sp.Variable([Dim.X],
                           values=np.array([0.1, 0.2]),
                           unit=sp.units.m)
    assert sp.abs(var) == expected


def test_dot():
    a = sp.Variable(dims=[Dim.X],
                    values=[[1, 0, 0], [0, 1, 0]],
                    unit=sp.units.m)
    b = sp.Variable(dims=[Dim.X],
                    values=[[1, 0, 0], [1, 0, 0]],
                    unit=sp.units.m)
    expected = sp.Variable([Dim.X],
                           values=np.array([1.0, 0.0]),
                           unit=sp.units.m**2)
    assert sp.dot(a, b) == expected


def test_concatenate():
    var = sp.Variable([Dim.X], values=np.array([0.1, 0.2]), unit=sp.units.m)
    expected = sp.Variable([sp.Dim.X],
                           values=np.array([0.1, 0.2, 0.1, 0.2]),
                           unit=sp.units.m)
    assert sp.concatenate(var, var, Dim.X) == expected


def test_mean():
    var = sp.Variable([Dim.X, Dim.Y],
                      values=np.array([[0.1, 0.3], [0.2, 0.6]]),
                      unit=sp.units.m)
    expected = sp.Variable([Dim.X],
                           values=np.array([0.2, 0.4]),
                           unit=sp.units.m)
    assert sp.mean(var, Dim.Y) == expected


def test_norm():
    var = sp.Variable(dims=[Dim.X],
                      values=[[1, 0, 0], [3, 4, 0]],
                      unit=sp.units.m)
    expected = sp.Variable([Dim.X],
                           values=np.array([1.0, 5.0]),
                           unit=sp.units.m)
    assert sp.norm(var) == expected


def test_sqrt():
    var = sp.Variable([Dim.X], values=np.array([4.0, 9.0]), unit=sp.units.m**2)
    expected = sp.Variable([Dim.X],
                           values=np.array([2.0, 3.0]),
                           unit=sp.units.m)
    assert sp.sqrt(var) == expected


def test_sum():
    var = sp.Variable([Dim.X, Dim.Y],
                      values=np.array([[0.1, 0.3], [0.2, 0.6]]),
                      unit=sp.units.m)
    expected = sp.Variable([Dim.X],
                           values=np.array([0.4, 0.8]),
                           unit=sp.units.m)
    assert sp.sum(var, Dim.Y) == expected


def test_variance_acess():
    v = sp.Variable()
    assert v.variance is None
    assert v.variances is None


def test_set_variance():
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sp.Variable(dims=[Dim.X, Dim.Y], values=values)
    expected = sp.Variable(dims=[Dim.X, Dim.Y],
                           values=values,
                           variances=variances)

    assert var.variances is None
    assert var != expected

    var.variances = variances

    assert var.variances is not None
    assert var == expected


def test_copy_variance():
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sp.Variable(dims=[Dim.X, Dim.Y], values=values)
    expected = sp.Variable(dims=[Dim.X, Dim.Y],
                           values=values,
                           variances=variances)

    assert var.variances is None
    assert var != expected

    var.variances = expected.variances

    assert var.variances is not None
    assert var == expected


def test_set_variance_convert_dtype():
    values = np.random.rand(2, 3)
    variances = np.arange(6).reshape(2, 3)
    assert variances.dtype == np.int
    var = sp.Variable(dims=[Dim.X, Dim.Y], values=values)
    expected = sp.Variable(dims=[Dim.X, Dim.Y],
                           values=values,
                           variances=variances)

    assert var.variances is None
    assert var != expected

    var.variances = variances

    assert var.variances is not None
    assert var == expected


def test_sum_mean():
    var = sp.Variable([Dim.X], values=np.ones(5).astype(np.int64))
    assert sp.sum(var, Dim.X) == sp.Variable(5)
    var = sp.Variable([Dim.X], values=np.arange(6).astype(np.int64))
    assert sp.mean(var, Dim.X) == sp.Variable(2.5)


def test_make_variable_from_unit_scalar_mult_div():
    var = sp.Variable()
    var.unit = sp.units.m
    assert var == 0.0 * sp.units.m
    var.unit = sp.units.m**(-1)
    assert var == 0.0 / sp.units.m

    # TODO change next string with normal constructor
    # for 0D Variable the it introduced
    v = sp.Variable([sp.Dim.X], values=np.array([0]), dtype=np.float32)
    var = sp.Variable(v[sp.Dim.X, 0])
    var.unit = sp.units.m
    assert var == np.float32(0.0) * sp.units.m
    var.unit = sp.units.m**(-1)
    assert var == np.float32(0.0) / sp.units.m
