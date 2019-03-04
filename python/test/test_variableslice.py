import unittest

from dataset import *
import numpy as np


class TestVariableSlice(unittest.TestCase):

    def setUp(self):
        self._a = Variable(Data.Value,[Dim.X], np.arange(1,10,dtype=float)) 
        self._b = Variable(Data.Value,[Dim.X], np.arange(1,10,dtype=float)) 
        self._a_slice = self._a[(Dim.X, slice(0, len(self._a)))]
        self._b_slice = self._b[(Dim.X, slice(0, len(self._b)))]
        
    def test_type(self):
        data_slice = self._a_slice 
        self.assertEqual(type(data_slice), dataset.VariableSlice)

    def test_variable_type(self):
        coord_var = Variable(Coord.X, [Dim.X], np.arange(10))
        coord_slice = coord_var[(Dim.X, slice(0))] 
        self.assertTrue(coord_slice.is_coord)
    
    def test_binary_operations(self):
        a = self._a_slice 
        b = self._b_slice 

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
    

if __name__ == '__main__':
    unittest.main()
