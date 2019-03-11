import unittest

from dataset import *
import numpy as np
import operator

class TestDatasetSlice(unittest.TestCase):

    def setUp(self):
        d = Dataset()
        d[Coord.X] = ([Dim.X], np.arange(10))
        d[Data.Value, "a"] = ([Dim.X], np.arange(10)) 
        d[Data.Value, "b"] = ([Dim.X], np.arange(10)) 
        self._d = d
    
    def test_type(self):
        ds_slice = self._d.subset["a"]
        self.assertEqual(type(ds_slice), dataset.DatasetSlice)

    def test_extract_slice(self):
        ds_slice = self._d.subset["a"]
        self.assertEqual(type(ds_slice), dataset.DatasetSlice)
        # We should have just one data variable
        self.assertEqual(1, len([var for var in ds_slice if var.is_data]))
        # We should have just one coord variable
        self.assertEqual(1, len([var for var in ds_slice if var.is_coord]))
        self.assertEqual(2, len(ds_slice))

    def test_slice_back_ommit_range(self):
        sl = self._d[Dim.X, 1:-1][Data.Value, "a"].numpy
        ref = np. array([1, 2, 3, 4, 5, 6, 7, 8], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertEqual(np.allclose(sl, ref), True)
        # omitting range end
        sl = self._d[Dim.X, 1:][Data.Value, "b"].numpy
        ref = np. array([1, 2, 3, 4, 5, 6, 7, 8, 9], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertEqual(np.allclose(sl, ref), True)
        # omitting range begin
        sl = self._d[Dim.X, :-1][Data.Value, "a"].numpy
        ref = np. array([0, 1, 2, 3, 4, 5, 6, 7, 8], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertEqual(np.allclose(sl, ref), True)
        # omitting range both begin and end
        sl = self._d[Dim.X, :][Data.Value, "b"].numpy
        ref = np. array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertEqual(np.allclose(sl, ref), True)

    def test_slice_single_index(self):
        self.assertEqual(self._d[Dim.X, -4][Data.Value, "a"].numpy,
                         self._d[Dim.X, 6][Data.Value, "a"].numpy)
        self.assertEqual(self._d[Data.Value, "a"][Dim.X, -3].numpy,
                         self._d[Data.Value, "a"][Dim.X, 7].numpy)


    def test_range_based_slice(self):
        subset = slice(1,4,1)
        # Create slice
        ds_slice = self._d[Dim.X,subset]
        # Test via variable_slice
        self.assertEqual(len(ds_slice[Coord.X]), len(range(subset.start, subset.stop, subset.step)))

    def _apply_test_op(self, op, a, b, data, lh_var_name="a", rh_var_name="b"):
        # Assume numpy operations are correct as comparitor
        op(data,b[Data.Value, rh_var_name].numpy)
        op(a,b)
        self.assertTrue(np.array_equal(a[Data.Value, lh_var_name].numpy, data))

    def test_binary_operations(self):
        d = Dataset()
        d[Coord.X] = ([Dim.X], np.arange(10))
        d[Data.Value, "a"] = ([Dim.X], np.arange(10, dtype='float64'))
        d[Data.Value, "b"] = ([Dim.X], np.arange(10, dtype='float64'))
        a = d.subset("a")
        b = d.subset("b")
        data = np.copy(a[Data.Value, "a"].numpy)
        c = a + b
        # Variables "a" and "b" added despite different names
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data + data))
        c = a - b
        # Variables "a" and "b" subtracted despite different names
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data - data))


        #TODO. resolve issues with times_equals and binary_op_equals preventing implementation of * and / variants

        c = a + 2.0
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data + 2.0))
        c = a - b
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data - data))
        c = a - 2.0
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data - 2.0))
        c = a * 2.0
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data * 2.0))
        c = a / 2.0
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data / 2.0))

        self._apply_test_op(operator.iadd, a, b, data)
        self._apply_test_op(operator.isub, a, b, data)
        # TODO problem described above need inplace operators
        # Only demonstrate behaviour where variable names are sames across operands
        b = d.subset("a")
        data = np.copy(a[Data.Value, "a"].numpy)
        self._apply_test_op(operator.imul, a, b, data, lh_var_name="a", rh_var_name="a")

if __name__ == '__main__':
    unittest.main()
