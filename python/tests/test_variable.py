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
    assert var.dtype == sp.dtype.double
    assert var.unit == sp.units.dimensionless
    assert var.value == 0.0


def test_create_default_dtype():
    var = sp.Variable([sp.Dim.X], [4])
    assert var.dtype == sp.dtype.double


def test_create_with_dtype():
    var = sp.Variable(labels=[Dim.X], shape=[2], dtype=sp.dtype.float)
    assert var.dtype == sp.dtype.float


def test_create_with_numpy_dtype():
    var = sp.Variable(labels=[Dim.X], shape=[2], dtype=np.dtype(np.float32))
    assert var.dtype == sp.dtype.float


def test_create_with_variances():
    assert not sp.Variable(labels=[Dim.X], shape=[2]).has_variances
    assert not sp.Variable(labels=[Dim.X], shape=[
                           2], variances=False).has_variances
    assert sp.Variable(labels=[Dim.X], shape=[2], variances=True).has_variances


def test_create_sparse():
    var = sp.Variable([sp.Dim.X, sp.Dim.Y], [4, sp.Dimensions.Sparse])
    assert var.dtype == sp.dtype.double
    assert var.sparse
    assert len(var.values) == 4
    for vals in var.values:
        assert len(vals) == 0


def test_create_from_numpy_1d():
    var = sp.Variable([sp.Dim.X], np.arange(4.0))
    assert var.dtype == sp.dtype.double
    np.testing.assert_array_equal(var.values, np.arange(4))


def test_create_from_numpy_1d_bool():
    var = sp.Variable(labels=[sp.Dim.X], values=np.array([True, False, True]))
    assert var.dtype == sp.dtype.bool
    np.testing.assert_array_equal(var.values, np.array([True, False, True]))


def test_create_with_variances_from_numpy_1d():
    var = sp.Variable([sp.Dim.X], values=np.arange(4.0),
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


def test_create_scalar_quantity():
    var = sp.Variable(1.2, unit=sp.units.m)
    assert var.value == 1.2
    assert var.dims == []
    assert var.dtype == sp.dtype.double
    assert var.unit == sp.units.m


def test_operation_with_scalar_quantity():
    reference = sp.Variable([sp.Dim.X],
                            np.arange(4.0) * 1.5)
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


def test_1D_scalar_access_fail():
    var = sp.Variable([Dim.X], (1,))
    with pytest.raises(RuntimeError):
        assert var.value == 0.0
    with pytest.raises(RuntimeError):
        var.value = 1.2


def test_1D_access():
    var = sp.Variable([Dim.X], (2,))
    assert len(var.values) == 2
    assert var.values.shape == (2,)
    var.values[1] = 1.2
    assert var.values[1] == 1.2


def test_1D_access_bad_shape_fail():
    var = sp.Variable([Dim.X], (2,))
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
    assert len(vals0) == 1


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
    var = sp.Variable([sp.Dim.X, sp.Dim.Y], [
                      4, sp.Dimensions.Sparse], dtype=sp.dtype.float)
    var[Dim.X, 0].values = np.arange(4)
    assert len(var[Dim.X, 0].values) == 4


def test_sparse_setitem_int64_t():
    var = sp.Variable([sp.Dim.X, sp.Dim.Y], [
                      4, sp.Dimensions.Sparse], dtype=sp.dtype.int64)
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
    var = sp.Variable([Dim.X], (4,), dtype=np.dtype(np.float64))
    assert var.dtype == sp.dtype.double
    var = sp.Variable([Dim.X], (4,), dtype=np.dtype(np.float32))
    assert var.dtype == sp.dtype.float
    var = sp.Variable([Dim.X], (4,), dtype=np.dtype(np.int64))
    assert var.dtype == sp.dtype.int64
    var = sp.Variable([Dim.X], (4,), dtype=np.dtype(np.int32))
    assert var.dtype == sp.dtype.int32


def test_get_slice():
    var = sp.Variable([Dim.X, Dim.Y], values=np.arange(0, 8).reshape(2, 4))
    var_slice = var[Dim.X, 1:2]
    assert var_slice == sp.Variable(
        [Dim.X, Dim.Y], values=np.arange(4, 8).reshape(1, 4))


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
