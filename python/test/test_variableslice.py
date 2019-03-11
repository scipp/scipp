import unittest

from dataset import *
import numpy as np
import operator


class TestVariableSlice(unittest.TestCase):

    def setUp(self):
        self._a = Variable(Data.Value,[Dim.X], np.arange(1,10,dtype=float))
        self._b = Variable(Data.Value,[Dim.X], np.arange(1,10,dtype=float))

    def test_type(self):
        variable_slice = self._a[Dim.X, :]
        self.assertEqual(type(variable_slice), dataset.VariableSlice)

    def test_variable_type(self):
        coord_var = Variable(Coord.X, [Dim.X], np.arange(10))
        coord_slice = coord_var[Dim.X, :]
        self.assertTrue(coord_slice.is_coord)

    def _apply_test_op(self, op, a, b, data):
        op(a,b)
        # Assume numpy operations are correct as comparitor
        op(data,b.numpy)
        self.assertTrue(np.array_equal(a.numpy, data))

    def test_binary_operations(self):
        a = self._a[Dim.X, :]
        b = self._b[Dim.X, :]

        data = np.copy(a.numpy)
        c = a + b
        self.assertEqual(type(c), dataset.Variable)
        self.assertTrue(np.array_equal(c.numpy, data+data))
        c = a - b
        self.assertTrue(np.array_equal(c.numpy, data-data))
        c = a * b
        self.assertTrue(np.array_equal(c.numpy, data*data))
        c = a / b
        self.assertTrue(np.array_equal(c.numpy, data/data))

        self._apply_test_op(operator.iadd, a, b, data)
        self._apply_test_op(operator.isub, a, b, data)
        self._apply_test_op(operator.imul, a, b, data)
        self._apply_test_op(operator.itruediv, a, b, data)

        # Equal
        self.assertEqual(a, b)
        self.assertEqual(b, a)
        # Not equal
        self.assertNotEqual(a, c)
        self.assertNotEqual(c, a)

if __name__ == '__main__':
    unittest.main()
