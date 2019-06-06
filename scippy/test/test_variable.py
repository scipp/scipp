# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import scippy as sp
from scippy import Dim
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
    assert var.dims == sp.Dimensions()
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


def test_create_from_numpy_1d():
    var = sp.Variable([sp.Dim.X], np.arange(4.0))
    assert var.dtype == sp.dtype.double
    np.testing.assert_array_equal(var.numpy, np.arange(4))


def test_create_from_numpy_1d_bool():
    var = sp.Variable(labels=[sp.Dim.X], values=np.array([True, False, True]))
    assert var.dtype == sp.dtype.bool
    np.testing.assert_array_equal(var.numpy, np.array([True, False, True]))


def test_create_scalar():
    var = sp.Variable(1.2)
    assert var.value == 1.2
    assert var.dims == sp.Dimensions()
    assert var.dtype == sp.dtype.double
    assert var.unit == sp.units.dimensionless


def test_create_scalar_quantity():
    var = sp.Variable(1.2, unit=sp.units.m)
    assert var.value == 1.2
    assert var.dims == sp.Dimensions()
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
    assert var.values[0] == 1.2


def test_1D_scalar_access_fail():
    var = sp.Variable([sp.Dim.X], (1,))
    with pytest.raises(RuntimeError):
        assert var.value == 0.0
    with pytest.raises(RuntimeError):
        var.value = 1.2


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


#def test_slicing(self):
#    var = sp.Variable([sp.Dim.X], np.arange(0, 3))
#    var_slice = var[(sp.Dim.X, slice(0, 2))]
#    self.assertTrue(isinstance(var_slice, sp.VariableSlice))
#    self.assertEqual(len(var_slice), 2)
#    self.assertTrue(np.array_equal(var_slice.numpy, np.array([0, 1])))
#
#
#def test_binary_plus(self):
#    a, b, a_slice, b_slice, data = make_variables()
#    c = a + b
#    self.assertTrue(np.array_equal(c.numpy, data + data))
#    c = a + 2.0
#    self.assertTrue(np.array_equal(c.numpy, data + 2.0))
#    c = a + b_slice
#    self.assertTrue(np.array_equal(c.numpy, data + data))
#    c += b
#    self.assertTrue(np.array_equal(c.numpy, data + data + data))
#    c += b_slice
#    self.assertTrue(np.array_equal(c.numpy, data + data + data + data))
#    c = 3.5 + c
#    self.assertTrue(np.array_equal(c.numpy, data + data + data + data + 3.5))
#
#
#def test_binary_minus(self):
#    a, b, a_slice, b_slice, data = make_variables()
#    c = a - b
#    self.assertTrue(np.array_equal(c.numpy, data - data))
#    c = a - 2.0
#    self.assertTrue(np.array_equal(c.numpy, data - 2.0))
#    c = a - b_slice
#    self.assertTrue(np.array_equal(c.numpy, data - data))
#    c -= b
#    self.assertTrue(np.array_equal(c.numpy, data - data - data))
#    c -= b_slice
#    self.assertTrue(np.array_equal(c.numpy, data - data - data - data))
#    c = 3.5 - c
#    self.assertTrue(np.array_equal(c.numpy, 3.5 - data + data + data + data))
#
#
#def test_binary_multiply(self):
#    a, b, a_slice, b_slice, data = make_variables()
#    c = a * b
#    self.assertTrue(np.array_equal(c.numpy, data * data))
#    c = a * 2.0
#    self.assertTrue(np.array_equal(c.numpy, data * 2.0))
#    c = a * b_slice
#    self.assertTrue(np.array_equal(c.numpy, data * data))
#    c *= b
#    self.assertTrue(np.array_equal(c.numpy, data * data * data))
#    c *= b_slice
#    self.assertTrue(np.array_equal(c.numpy, data * data * data * data))
#    c = 3.5 * c
#    self.assertTrue(np.array_equal(c.numpy, data * data * data * data * 3.5))
#
#
#def test_binary_divide(self):
#    a, b, a_slice, b_slice, data = make_variables()
#    c = a / b
#    self.assertTrue(np.array_equal(c.numpy, data / data))
#    c = a / 2.0
#    self.assertTrue(np.array_equal(c.numpy, data / 2.0))
#    c = a / b_slice
#    self.assertTrue(np.array_equal(c.numpy, data / data))
#    c /= b
#    self.assertTrue(np.array_equal(c.numpy, data / data / data))
#    c /= b_slice
#    self.assertTrue(np.array_equal(c.numpy, data / data / data / data))
#
#
#def test_binary_equal(self):
#    a, b, a_slice, b_slice, data = make_variables()
#    self.assertEqual(a, b)
#    self.assertEqual(a, a_slice)
#    self.assertEqual(a_slice, b_slice)
#    self.assertEqual(b, a)
#    self.assertEqual(b_slice, a)
#    self.assertEqual(b_slice, a_slice)
#
#
#def test_binary_not_equal(self):
#    a, b, a_slice, b_slice, data = make_variables()
#    c = a + b
#    self.assertNotEqual(a, c)
#    self.assertNotEqual(a_slice, c)
#    self.assertNotEqual(c, a)
#    self.assertNotEqual(c, a_slice)


if __name__ == '__main__':
    unittest.main()
