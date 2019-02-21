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

    def test_variable_type(self):
        var_coord = Variable(Coord.X, [Dim.X], (4,))
        var_data = Variable(Data.Value, [Dim.X], (4,))

        self.assertTrue(var_coord.is_coord)
        self.assertTrue(var_data.is_data)

    def test_create_dtype(self):
        print("start")
        var = Variable(Coord.X, [Dim.X], np.arange(4))
        var = Variable(Coord.X, [Dim.X], np.arange(4).astype(np.int32))
        var = Variable(Coord.X, [Dim.X], np.arange(4).astype(np.float64))
        var = Variable(Coord.X, [Dim.X], np.arange(4).astype(np.float32))
        #var = Variable(Coord.X, [Dim.X], ['a', 'bb', 'ccc', 'dddd'])
        print("end")
        var = Variable(Coord.X, [Dim.X], (4,), dtype=np.dtype(np.float64))
        self.assertEqual(var.numpy.dtype, np.dtype(np.float64))
        var = Variable(Coord.X, [Dim.X], (4,), dtype=np.dtype(np.float32))
        self.assertEqual(var.numpy.dtype, np.dtype(np.float32))
        var = Variable(Coord.X, [Dim.X], (4,), dtype=np.dtype(np.int64))
        self.assertEqual(var.numpy.dtype, np.dtype(np.int64))
        var = Variable(Coord.X, [Dim.X], (4,), dtype=np.dtype(np.int32))
        self.assertEqual(var.numpy.dtype, np.dtype(np.int32))

    def test_coord_set_name_fails(self):
        var = Variable(Coord.X, [Dim.X], np.arange(4))
        with self.assertRaises(RuntimeError):
            var.name = "data"

    def test_set_name(self):
        var = Variable(Data.Value, [Dim.X], np.arange(4))
        var.name = "data"
        self.assertEqual(var.name, "data")

    def test_repr(self):
        var = Variable(Coord.X, [Dim.X], np.arange(1))
        self.assertEqual(repr(var), "Variable(Coord.X, '',( Dim.X ), int64)\n")
        var = Variable(Data.Value, [Dim.X], np.arange(1))
        self.assertEqual(repr(var), "Variable(Data.Value, '',( Dim.X ), int64)\n")
        var = Variable(Coord.SpectrumNumber, [Dim.X], np.arange(1))
        self.assertEqual(repr(var), "Variable(Coord.SpectrumNumber, '',( Dim.X ), int64)\n")
        var = Variable(Coord.Mask, [Dim.X], np.arange(1))
        self.assertEqual(repr(var), "Variable(Coord.Mask, '',( Dim.X ), int64)\n")
    
    def test_binary_operations(self):
        # TODO units check
        data = np.arange(1, 4, dtype=float)
        a = Variable(Data.Value, [Dim.X], data)
        b = Variable(Data.Value, [Dim.X], data)
        # Plus
        c = a + b
        self.assertTrue(np.array_equal(c.numpy, data+data))
        c += b
        self.assertTrue(np.array_equal(c.numpy, data+data+data))
        # Minus
        c = a - b
        self.assertTrue(np.array_equal(c.numpy, data-data))
        c -= b
        self.assertTrue(np.array_equal(c.numpy, data-data-data))
        # Multiply
        c = a * b
        self.assertTrue(np.array_equal(c.numpy, data*data))
        c *= b
        self.assertTrue(np.array_equal(c.numpy, data*data*data))
        # Divide
        c = a / b
        self.assertTrue(np.array_equal(c.numpy, data/data))
        c /= b
        self.assertTrue(np.array_equal(c.numpy, data/data/data))
        c = a + 2.0 
        self.assertTrue(np.array_equal(c.numpy, data+2.0))
        c = a - 2.0 
        self.assertTrue(np.array_equal(c.numpy, data-2.0))
        c = a * 2.0 
        self.assertTrue(np.array_equal(c.numpy, data*2.0))
        c = a / 2.0 
        self.assertTrue(np.array_equal(c.numpy, data/2.0))

if __name__ == '__main__':
    unittest.main()
