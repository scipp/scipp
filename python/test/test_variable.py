import unittest

from dataset import *
import numpy as np

class TestVariable(unittest.TestCase):
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
        var = Variable(Coord.X, [Dim.X], np.arange(4))
        var = Variable(Coord.X, [Dim.X], np.arange(4).astype(np.int32))
        var = Variable(Coord.X, [Dim.X], np.arange(4).astype(np.float64))
        var = Variable(Coord.X, [Dim.X], np.arange(4).astype(np.float32))
        #var = Variable(Coord.X, [Dim.X], ['a', 'bb', 'ccc', 'dddd'])
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

if __name__ == '__main__':
    unittest.main()
