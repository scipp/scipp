import unittest

from dataset import *
import numpy as np


class TestVariableSlice(unittest.TestCase):

    def setUp(self):
        d = Dataset()
        d[Coord.X] = ([Dim.X], np.arange(10))
        d[Data.Value, "a"] = ([Dim.X], np.arange(1,10,dtype=float)) 
        d[Data.Value, "b"] = ([Dim.X], np.arange(1,10,dtype=float)) 
        self._d = d
        self._dslice = d["a"]

    def test_type(self):
        coord_slice = self._dslice[Coord.X]
        self.assertEqual(type(coord_slice), dataset.VariableSlice)
    
    def test_builtins(self):
        coord_slice = self._dslice[Coord.X]
        self.assertEqual(len(coord_slice), len(self._d[Coord.X]))

    def test_variable_type(self):
        coord_slice = self._dslice[Coord.X]
        self.assertTrue(coord_slice.is_coord)
        data_slice = self._dslice[Data.Value, "a"]
        self.assertTrue(data_slice.is_data)
    
    def test_binary_operations(self):
        a = self._dslice[Data.Value, "a"] 
        b = self._d[Data.Value, "b"] 

        data = a.numpy
        c = a + b
        self.assertEqual(type(c), dataset.Variable)
        self.assertTrue(np.array_equal(c.numpy, data+data))
        c = a - b
        self.assertTrue(np.array_equal(c.numpy, data-data))
        c = a * b
        self.assertTrue(np.array_equal(c.numpy, data*data))
        c = a / b
        self.assertTrue(np.array_equal(c.numpy, data/data))

        a += b
        a -= b
        a *= b
        a /= b


if __name__ == '__main__':
    unittest.main()
