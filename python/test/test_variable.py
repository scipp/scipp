import unittest

from dataset import *
import numpy as np

class TestVariable(unittest.TestCase):
    def test_create_coord(self):
        var = Variable(Coord.X, [Dim.X], np.arange(4))
        self.assertEqual(var.name, "")
        self.assertEqual(var.numpy.dtype, np.dtype(np.float64))
        np.testing.assert_array_equal(var.numpy, np.arange(4))

    def test_create_coord_different_default_dtype(self):
        var = Variable(Coord.Mask, [Dim.X], np.arange(4))
        self.assertEqual(var.name, "")
        self.assertEqual(var.numpy.dtype, np.dtype(np.int8))
        np.testing.assert_array_equal(var.numpy, np.arange(4))

    def test_create_data(self):
        var = Variable(Data.Value, [Dim.X], np.arange(4))
        self.assertEqual(var.name, "")
        self.assertEqual(var.numpy.dtype, np.dtype(np.float64))
        np.testing.assert_array_equal(var.numpy, np.arange(4))

    def test_create_default_init(self):
        var = Variable(Coord.X, [Dim.X], (4,))
        self.assertEqual(var.name, "")

    def test_create_dtype(self):
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

if __name__ == '__main__':
    unittest.main()
