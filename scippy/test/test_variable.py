# @file
# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
import unittest

from scippy import *
import numpy as np

def make_variables():
    data = np.arange(1, 4, dtype=float)
    a = Variable(Data.Value, [Dim.X], data)
    b = Variable(Data.Value, [Dim.X], data)
    a_slice = a[Dim.X, :]
    b_slice = b[Dim.X, :]
    return a, b, a_slice, b_slice, data

class TestVariable(unittest.TestCase):

    def test_builtins(self):
        # Test builtin support
        var = Variable(Coord.X, [Dim.X], np.arange(4.0))
        self.assertEqual(len(var), 4)

    def test_create_coord(self):
        var = Variable(Coord.X, [Dim.X], np.arange(4.0))
        self.assertEqual(var.name, "")
        self.assertEqual(var.numpy.dtype, np.dtype(np.float64))
        np.testing.assert_array_equal(var.numpy, np.arange(4))

    def test_create_coord_different_default_dtype(self):
        var = Variable(Coord.Mask, [Dim.X], np.array([True, False, True, False]))
        self.assertEqual(var.name, "")
        self.assertEqual(var.numpy.dtype, np.dtype(np.bool))
        np.testing.assert_array_equal(var.numpy, np.array([True, False, True, False]))

    def test_create_data(self):
        var = Variable(Data.Value, [Dim.X], np.arange(4.0))
        self.assertEqual(var.name, "")
        self.assertEqual(var.numpy.dtype, np.dtype(np.float64))
        np.testing.assert_array_equal(var.numpy, np.arange(4))

    def test_create_default_init(self):
        var = Variable(Coord.X, [Dim.X], (4,))
        self.assertEqual(var.name, "")

    def test_create_scalar(self):
        var = Variable(1.2)
        self.assertEqual(var.scalar, 1.2)
        self.assertEqual(var.name, '')
        self.assertEqual(var.dimensions, Dimensions())
        self.assertEqual(var.numpy.dtype, np.dtype(np.float64))
        self.assertEqual(var.unit, units.dimensionless)

    def test_create_scalar_quantity(self):
        var = Variable(1.2, unit=units.m)
        self.assertEqual(var.scalar, 1.2)
        self.assertEqual(var.name, '')
        self.assertEqual(var.dimensions, Dimensions())
        self.assertEqual(var.numpy.dtype, np.dtype(np.float64))
        self.assertEqual(var.unit, units.m)

    def test_operation_with_scalar_quantity(self):
        reference = Variable(Data.Value, [Dim.X], np.arange(4.0) * 1.5)
        reference.unit = units.kg

        var = Variable(Data.Value, [Dim.X], np.arange(4.0))
        var *= Variable(1.5, unit=units.kg)
        self.assertEqual(var, reference)

    def test_0D_scalar_access(self):
        var = Variable(Coord.X, [], ())
        self.assertEqual(var.scalar, 0.0)
        var.scalar = 1.2
        self.assertEqual(var.scalar, 1.2)
        self.assertEqual(var.data[0], 1.2)

    def test_1D_scalar_access_fail(self):
        var = Variable(Coord.X, [Dim.X], (1,))
        with self.assertRaisesRegex(RuntimeError, "Expected dimensions {}, got {{Dim::X, 1}}."):
            var.scalar = 1.2

    def test_variable_type(self):
        var_coord = Variable(Coord.X, [Dim.X], (4,))
        var_data = Variable(Data.Value, [Dim.X], (4,))

        self.assertTrue(var_coord.is_coord)
        self.assertTrue(var_data.is_data)

    def test_create_dtype(self):
        var = Variable(Coord.X, [Dim.X], np.arange(4))
        var = Variable(Coord.X, [Dim.X], np.arange(4).astype(np.int32))
        var = Variable(Coord.X, [Dim.X], np.arange(4).astype(np.float64))
        var = Variable(Coord.X, [Dim.X], np.arange(4).astype(np.float32))
        var = Variable(Coord.X, [Dim.X], (4,), dtype=np.dtype(np.float64))
        self.assertEqual(var.numpy.dtype, np.dtype(np.float64))
        var = Variable(Coord.X, [Dim.X], (4,), dtype=np.dtype(np.float32))
        self.assertEqual(var.numpy.dtype, np.dtype(np.float32))
        var = Variable(Coord.X, [Dim.X], (4,), dtype=np.dtype(np.int64))
        self.assertEqual(var.numpy.dtype, np.dtype(np.int64))
        var = Variable(Coord.X, [Dim.X], (4,), dtype=np.dtype(np.int32))
        self.assertEqual(var.numpy.dtype, np.dtype(np.int32))

    def test_coord_set_name(self):
        var = Variable(Coord.X, [Dim.X], np.arange(4))
        # We can set a name for coordinates, but not that operations will typically
        # not look for a named coordinate. In particular it will not be considered
        # a "dimension coordinate".
        var.name = "x"
        self.assertEqual(var.name, "x")

    def test_set_name(self):
        var = Variable(Data.Value, [Dim.X], np.arange(4))
        var.name = "data"
        self.assertEqual(var.name, "data")

    def test_slicing(self):
        var = Variable(Data.Value, [Dim.X], np.arange(0,3))
        var_slice = var[(Dim.X, slice(0, 2))]
        self.assertTrue(isinstance(var_slice, VariableSlice))
        self.assertEqual(len(var_slice), 2)
        self.assertTrue(np.array_equal(var_slice.numpy, np.array([0,1])))

    def test_binary_plus(self):
        a, b, a_slice, b_slice, data = make_variables()
        c = a + b
        self.assertTrue(np.array_equal(c.numpy, data+data))
        c = a + 2.0
        self.assertTrue(np.array_equal(c.numpy, data+2.0))
        c = a + b_slice
        self.assertTrue(np.array_equal(c.numpy, data+data))
        c += b
        self.assertTrue(np.array_equal(c.numpy, data+data+data))
        c += b_slice
        self.assertTrue(np.array_equal(c.numpy, data+data+data+data))
        c = 3.5 + c
        self.assertTrue(np.array_equal(c.numpy, data+data+data+data+3.5))

    def test_binary_minus(self):
        a, b, a_slice, b_slice, data = make_variables()
        c = a - b
        self.assertTrue(np.array_equal(c.numpy, data-data))
        c = a - 2.0
        self.assertTrue(np.array_equal(c.numpy, data-2.0))
        c = a - b_slice
        self.assertTrue(np.array_equal(c.numpy, data-data))
        c -= b
        self.assertTrue(np.array_equal(c.numpy, data-data-data))
        c -= b_slice
        self.assertTrue(np.array_equal(c.numpy, data-data-data-data))
        c = 3.5 - c
        self.assertTrue(np.array_equal(c.numpy, 3.5-data+data+data+data))

    def test_binary_multiply(self):
        a, b, a_slice, b_slice, data = make_variables()
        c = a * b
        self.assertTrue(np.array_equal(c.numpy, data*data))
        c = a * 2.0
        self.assertTrue(np.array_equal(c.numpy, data*2.0))
        c = a * b_slice
        self.assertTrue(np.array_equal(c.numpy, data*data))
        c *= b
        self.assertTrue(np.array_equal(c.numpy, data*data*data))
        c *= b_slice
        self.assertTrue(np.array_equal(c.numpy, data*data*data*data))
        c = 3.5 * c
        self.assertTrue(np.array_equal(c.numpy, data*data*data*data*3.5))

    def test_binary_divide(self):
        a, b, a_slice, b_slice, data = make_variables()
        c = a / b
        self.assertTrue(np.array_equal(c.numpy, data/data))
        c = a / 2.0
        self.assertTrue(np.array_equal(c.numpy, data/2.0))
        c = a / b_slice
        self.assertTrue(np.array_equal(c.numpy, data/data))
        c /= b
        self.assertTrue(np.array_equal(c.numpy, data/data/data))
        c /= b_slice
        self.assertTrue(np.array_equal(c.numpy, data/data/data/data))

    def test_binary_equal(self):
        a, b, a_slice, b_slice, data = make_variables()
        self.assertEqual(a, b)
        self.assertEqual(a, a_slice)
        self.assertEqual(a_slice, b_slice)
        self.assertEqual(b, a)
        self.assertEqual(b_slice, a)
        self.assertEqual(b_slice, a_slice)

    def test_binary_not_equal(self):
        a, b, a_slice, b_slice, data = make_variables()
        c = a + b
        self.assertNotEqual(a, c)
        self.assertNotEqual(a_slice, c)
        self.assertNotEqual(c, a)
        self.assertNotEqual(c, a_slice)

if __name__ == '__main__':
    unittest.main()
