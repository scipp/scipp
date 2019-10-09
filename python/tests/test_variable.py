# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import scipp as sc
from scipp import Dim
import numpy as np


def make_variables():
    data = np.arange(1, 4, dtype=float)
    a = sc.Variable([sc.Dim.X], data)
    b = sc.Variable([sc.Dim.X], data)
    a_slice = a[sc.Dim.X, :]
    b_slice = b[sc.Dim.X, :]
    return a, b, a_slice, b_slice, data


def test_create_default():
    var = sc.Variable()
    assert var.dims == []
    assert var.sparse_dim is None
    assert var.dtype == sc.dtype.double
    assert var.unit == sc.units.dimensionless
    assert var.value == 0.0


def test_create_default_dtype():
    var = sc.Variable([sc.Dim.X], [4])
    assert var.dtype == sc.dtype.double


def test_create_with_dtype():
    var = sc.Variable(dims=[Dim.X], shape=[2], dtype=sc.dtype.float)
    assert var.dtype == sc.dtype.float


def test_create_with_numpy_dtype():
    var = sc.Variable(dims=[Dim.X], shape=[2], dtype=np.dtype(np.float32))
    assert var.dtype == sc.dtype.float


def test_create_with_variances():
    assert sc.Variable(dims=[Dim.X], shape=[2]).variances is None
    assert sc.Variable(dims=[Dim.X], shape=[2],
                       variances=False).variances is None
    assert sc.Variable(dims=[Dim.X], shape=[2],
                       variances=True).variances is not None


def test_create_with_shape_and_variances():
    # If no values are given, variances must be Bool, cannot pass array.
    with pytest.raises(TypeError):
        sc.Variable(dims=[Dim.X], shape=[2], variances=np.arange(2))


def test_create_sparse():
    var = sc.Variable([sc.Dim.X, sc.Dim.Y], [4, sc.Dimensions.Sparse])
    assert var.dtype == sc.dtype.double
    assert var.sparse_dim == sc.Dim.Y
    assert len(var.values) == 4
    for vals in var.values:
        assert len(vals) == 0


def test_create_from_numpy_1d():
    var = sc.Variable([sc.Dim.X], np.arange(4.0))
    assert var.dtype == sc.dtype.double
    np.testing.assert_array_equal(var.values, np.arange(4))


def test_create_from_numpy_1d_bool():
    var = sc.Variable(dims=[sc.Dim.X], values=np.array([True, False, True]))
    assert var.dtype == sc.dtype.bool
    np.testing.assert_array_equal(var.values, np.array([True, False, True]))


def test_create_with_variances_from_numpy_1d():
    var = sc.Variable([sc.Dim.X],
                      values=np.arange(4.0),
                      variances=np.arange(4.0, 8.0))
    assert var.dtype == sc.dtype.double
    np.testing.assert_array_equal(var.values, np.arange(4))
    np.testing.assert_array_equal(var.variances, np.arange(4, 8))


def test_create_scalar():
    var = sc.Variable(1.2)
    assert var.value == 1.2
    assert var.dims == []
    assert var.dtype == sc.dtype.double
    assert var.unit == sc.units.dimensionless


def test_create_scalar_Dataset():
    dataset = sc.Dataset({'a': sc.Variable([sc.Dim.X], np.arange(4.0))})
    var = sc.Variable(dataset)
    assert var.value == dataset
    assert var.dims == []
    assert var.dtype == sc.dtype.Dataset
    assert var.unit == sc.units.dimensionless


def test_create_scalar_quantity():
    var = sc.Variable(1.2, unit=sc.units.m)
    assert var.value == 1.2
    assert var.dims == []
    assert var.dtype == sc.dtype.double
    assert var.unit == sc.units.m


def test_create_via_unit():
    expected = sc.Variable(1.2, unit=sc.units.m)
    var = 1.2 * sc.units.m
    assert var == expected


def test_create_1D_string():
    var = sc.Variable(dims=[Dim.Row], values=['a', 'bb'], unit=sc.units.m)
    assert len(var.values) == 2
    assert var.values[0] == 'a'
    assert var.values[1] == 'bb'
    assert var.dims == [Dim.Row]
    assert var.dtype == sc.dtype.string
    assert var.unit == sc.units.m


def test_create_1D_vector_3_double():
    var = sc.Variable(dims=[Dim.X],
                      values=[[1, 2, 3], [4, 5, 6]],
                      unit=sc.units.m)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == [Dim.X]
    assert var.dtype == sc.dtype.vector_3_double
    assert var.unit == sc.units.m


def test_create_2D_inner_size_3():
    var = sc.Variable(dims=[Dim.X, Dim.Y],
                      values=np.arange(6.0).reshape(2, 3),
                      unit=sc.units.m)
    assert var.shape == [2, 3]
    np.testing.assert_array_equal(var.values[0], [0, 1, 2])
    np.testing.assert_array_equal(var.values[1], [3, 4, 5])
    assert var.dims == [Dim.X, Dim.Y]
    assert var.dtype == sc.dtype.double
    assert var.unit == sc.units.m


def test_operation_with_scalar_quantity():
    reference = sc.Variable([sc.Dim.X], np.arange(4.0) * 1.5)
    reference.unit = sc.units.kg

    var = sc.Variable([sc.Dim.X], np.arange(4.0))
    var *= sc.Variable(1.5, unit=sc.units.kg)
    assert var == reference


def test_0D_scalar_access():
    var = sc.Variable()
    assert var.value == 0.0
    var.value = 1.2
    assert var.value == 1.2
    assert var.values.shape == ()
    assert var.values == 1.2


def test_0D_scalar_string():
    var = sc.Variable(value='a')
    assert var.value == 'a'
    var.value = 'b'
    assert var == sc.Variable(value='b')


def test_1D_scalar_access_fail():
    var = sc.Variable([Dim.X], (1, ))
    with pytest.raises(RuntimeError):
        assert var.value == 0.0
    with pytest.raises(RuntimeError):
        var.value = 1.2


def test_1D_access():
    var = sc.Variable([Dim.X], (2, ))
    assert len(var.values) == 2
    assert var.values.shape == (2, )
    var.values[1] = 1.2
    assert var.values[1] == 1.2


def test_1D_set_from_list():
    var = sc.Variable([Dim.X], (2, ))
    var.values = [1.0, 2.0]
    assert var == sc.Variable([Dim.X], values=[1.0, 2.0])


def test_1D_string():
    var = sc.Variable([Dim.X], values=['a', 'b'])
    assert len(var.values) == 2
    assert var.values[0] == 'a'
    assert var.values[1] == 'b'
    var.values = ['c', 'd']
    assert var == sc.Variable([Dim.X], values=['c', 'd'])


def test_1D_converting():
    var = sc.Variable([Dim.X], values=[1, 2])
    var.values = [3.3, 4.6]
    # floats get truncated
    assert var == sc.Variable([Dim.X], values=[3, 4])


def test_1D_dataset():
    var = sc.Variable([Dim.X], shape=(2, ), dtype=sc.dtype.Dataset)
    d1 = sc.Dataset({'a': 1.5 * sc.units.m})
    d2 = sc.Dataset({'a': 2.5 * sc.units.m})
    var.values = [d1, d2]
    assert var.values[0] == d1
    assert var.values[1] == d2


def test_1D_access_bad_shape_fail():
    var = sc.Variable([Dim.X], (2, ))
    with pytest.raises(RuntimeError):
        var.values = np.arange(3)


def test_2D_access():
    var = sc.Variable([Dim.X, Dim.Y], (2, 3))
    assert var.values.shape == (2, 3)
    assert len(var.values) == 2
    assert len(var.values[0]) == 3
    var.values[1] = 1.2  # numpy assigns to all elements in "slice"
    var.values[1][2] = 2.2
    assert var.values[1][0] == 1.2
    assert var.values[1][1] == 1.2
    assert var.values[1][2] == 2.2


def test_2D_access_bad_shape_fail():
    var = sc.Variable([Dim.X, Dim.Y], (2, 3))
    with pytest.raises(RuntimeError):
        var.values = np.ones(shape=(3, 2))


def test_2D_access_variances():
    var = sc.Variable([Dim.X, Dim.Y], (2, 3), variances=True)
    assert var.values.shape == (2, 3)
    assert var.variances.shape == (2, 3)
    var.values[1] = 1.2
    assert np.array_equal(var.variances, np.zeros(shape=(2, 3)))
    var.variances = np.ones(shape=(2, 3))
    assert np.array_equal(var.variances, np.ones(shape=(2, 3)))


def test_sparse_slice():
    var = sc.Variable([sc.Dim.X, sc.Dim.Y], [4, sc.Dimensions.Sparse])
    vals0 = var[Dim.X, 0].values
    assert len(vals0) == 0
    vals0.append(1.2)
    assert len(var[Dim.X, 0].values) == 1


def test_sparse_setitem():
    var = sc.Variable([sc.Dim.X, sc.Dim.Y], [4, sc.Dimensions.Sparse])
    var[Dim.X, 0].values = np.arange(4)
    assert len(var[Dim.X, 0].values) == 4


def test_sparse_setitem_sparse_fail():
    var = sc.Variable([sc.Dim.X, sc.Dim.Y], [4, sc.Dimensions.Sparse])
    with pytest.raises(RuntimeError):
        var.values = np.arange(3)


def test_sparse_setitem_shape_fail():
    var = sc.Variable([sc.Dim.X, sc.Dim.Y], [4, sc.Dimensions.Sparse])
    with pytest.raises(RuntimeError):
        var[Dim.X, 0].values = np.ones(shape=(3, 2))


def test_sparse_setitem_float():
    var = sc.Variable([sc.Dim.X, sc.Dim.Y], [4, sc.Dimensions.Sparse],
                      dtype=sc.dtype.float)
    var[Dim.X, 0].values = np.arange(4)
    assert len(var[Dim.X, 0].values) == 4


def test_sparse_setitem_int64_t():
    var = sc.Variable([sc.Dim.X, sc.Dim.Y], [4, sc.Dimensions.Sparse],
                      dtype=sc.dtype.int64)
    var[Dim.X, 0].values = np.arange(4)
    assert len(var[Dim.X, 0].values) == 4


def test_create_dtype():
    var = sc.Variable([Dim.X], values=np.arange(4))
    assert var.dtype == sc.dtype.int64
    var = sc.Variable([Dim.X], values=np.arange(4).astype(np.int32))
    assert var.dtype == sc.dtype.int32
    var = sc.Variable([Dim.X], values=np.arange(4).astype(np.float64))
    assert var.dtype == sc.dtype.double
    var = sc.Variable([Dim.X], values=np.arange(4).astype(np.float32))
    assert var.dtype == sc.dtype.float
    var = sc.Variable([Dim.X], (4, ), dtype=np.dtype(np.float64))
    assert var.dtype == sc.dtype.double
    var = sc.Variable([Dim.X], (4, ), dtype=np.dtype(np.float32))
    assert var.dtype == sc.dtype.float
    var = sc.Variable([Dim.X], (4, ), dtype=np.dtype(np.int64))
    assert var.dtype == sc.dtype.int64
    var = sc.Variable([Dim.X], (4, ), dtype=np.dtype(np.int32))
    assert var.dtype == sc.dtype.int32


def test_get_slice():
    var = sc.Variable([Dim.X, Dim.Y], values=np.arange(0, 8).reshape(2, 4))
    var_slice = var[Dim.X, 1:2]
    assert var_slice == sc.Variable([Dim.X, Dim.Y],
                                    values=np.arange(4, 8).reshape(1, 4))


def test_slicing():
    var = sc.Variable([sc.Dim.X], values=np.arange(0, 3))
    var_slice = var[(sc.Dim.X, slice(0, 2))]
    assert isinstance(var_slice, sc.VariableProxy)
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


def test_binary_or():
    a = sp.Variable(False)
    b = sp.Variable(True)
    a |= b
    assert a == sp.Variable(True)


def test_in_place_binary_or():
    a = sp.Variable(False)
    b = sp.Variable(True)
    assert (a | b) == sp.Variable(True)


def test_in_place_binary_with_scalar():
    v = sc.Variable([Dim.X], values=[10])
    copy = v.copy()

    v += 2
    v *= 2
    v -= 4
    v /= 2
    assert v == copy


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
    var = sc.Variable([Dim.X], values=np.array([0.1, -0.2]), unit=sc.units.m)
    expected = sc.Variable([Dim.X],
                           values=np.array([0.1, 0.2]),
                           unit=sc.units.m)
    assert sc.abs(var) == expected


def test_dot():
    a = sc.Variable(dims=[Dim.X],
                    values=[[1, 0, 0], [0, 1, 0]],
                    unit=sc.units.m)
    b = sc.Variable(dims=[Dim.X],
                    values=[[1, 0, 0], [1, 0, 0]],
                    unit=sc.units.m)
    expected = sc.Variable([Dim.X],
                           values=np.array([1.0, 0.0]),
                           unit=sc.units.m**2)
    assert sc.dot(a, b) == expected


def test_concatenate():
    var = sc.Variable([Dim.X], values=np.array([0.1, 0.2]), unit=sc.units.m)
    expected = sc.Variable([sc.Dim.X],
                           values=np.array([0.1, 0.2, 0.1, 0.2]),
                           unit=sc.units.m)
    assert sc.concatenate(var, var, Dim.X) == expected


def test_mean():
    var = sc.Variable([Dim.X, Dim.Y],
                      values=np.array([[0.1, 0.3], [0.2, 0.6]]),
                      unit=sc.units.m)
    expected = sc.Variable([Dim.X],
                           values=np.array([0.2, 0.4]),
                           unit=sc.units.m)
    assert sc.mean(var, Dim.Y) == expected


def test_norm():
    var = sc.Variable(dims=[Dim.X],
                      values=[[1, 0, 0], [3, 4, 0]],
                      unit=sc.units.m)
    expected = sc.Variable([Dim.X],
                           values=np.array([1.0, 5.0]),
                           unit=sc.units.m)
    assert sc.norm(var) == expected


def test_sqrt():
    var = sc.Variable([Dim.X], values=np.array([4.0, 9.0]), unit=sc.units.m**2)
    expected = sc.Variable([Dim.X],
                           values=np.array([2.0, 3.0]),
                           unit=sc.units.m)
    assert sc.sqrt(var) == expected


def test_sum():
    var = sc.Variable([Dim.X, Dim.Y],
                      values=np.array([[0.1, 0.3], [0.2, 0.6]]),
                      unit=sc.units.m)
    expected = sc.Variable([Dim.X],
                           values=np.array([0.4, 0.8]),
                           unit=sc.units.m)
    assert sc.sum(var, Dim.Y) == expected


def test_variance_acess():
    v = sc.Variable()
    assert v.variance is None
    assert v.variances is None


def test_set_variance():
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sc.Variable(dims=[Dim.X, Dim.Y], values=values)
    expected = sc.Variable(dims=[Dim.X, Dim.Y],
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
    var = sc.Variable(dims=[Dim.X, Dim.Y], values=values)
    expected = sc.Variable(dims=[Dim.X, Dim.Y],
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
    var = sc.Variable(dims=[Dim.X, Dim.Y], values=values)
    expected = sc.Variable(dims=[Dim.X, Dim.Y],
                           values=values,
                           variances=variances)

    assert var.variances is None
    assert var != expected

    var.variances = variances

    assert var.variances is not None
    assert var == expected


def test_sum_mean():
    var = sc.Variable([Dim.X], values=np.ones(5).astype(np.int64))
    assert sc.sum(var, Dim.X) == sc.Variable(5)
    var = sc.Variable([Dim.X], values=np.arange(6).astype(np.int64))
    assert sc.mean(var, Dim.X) == sc.Variable(2.5)


def test_make_variable_from_unit_scalar_mult_div():
    var = sc.Variable()
    var.unit = sc.units.m
    assert var == 0.0 * sc.units.m
    var.unit = sc.units.m**(-1)
    assert var == 0.0 / sc.units.m

    var = sc.Variable(np.float32())
    var.unit = sc.units.m
    assert var == np.float32(0.0) * sc.units.m
    var.unit = sc.units.m**(-1)
    assert var == np.float32(0.0) / sc.units.m


def test_construct_0d_numpy():
    v = sc.Variable([sc.Dim.X], values=np.array([0]), dtype=np.float32)
    var = sc.Variable(v[sc.Dim.X, 0])
    assert var == sc.Variable(np.float32())


def test_construct_0d_native_python_types():
    assert sc.Variable(2).dtype == sc.dtype.int64
    assert sc.Variable(2.0).dtype == sc.dtype.double
    assert sc.Variable(True).dtype == sc.dtype.bool


def test_rename_dims():
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=[Dim.X, Dim.Y], values=values)
    zy = sc.Variable(dims=[Dim.Z, Dim.Y], values=values)
    xy.rename_dims({Dim.X: Dim.Z})
    assert xy == zy
