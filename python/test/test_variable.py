import unittest

from dataset import *
import numpy as np

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
    
    def test_binary_operations(self):
        # TODO units check
        data = np.arange(1, 4, dtype=float)
        a = Variable(Data.Value, [Dim.X], data)
        b = Variable(Data.Value, [Dim.X], data)
        a_slice = a[Dim.X, :]
        b_slice = b[Dim.X, :]
        # Plus
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
        # Minus
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
        # Multiply
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
        # Divide
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

if __name__ == '__main__':
    unittest.main()
