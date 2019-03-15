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

    def test_copy(self):
        import copy
        N = 6
        M = 4
        d1 = Dataset()
        d1[Coord.X] = ([Dim.X], np.arange(N+1).astype(np.float64))
        d1[Coord.Y] = ([Dim.Y], np.arange(M+1).astype(np.float64))
        arr1 = np.arange(N*M).reshape(N,M).astype(np.float64) + 1
        d1[Data.Value, "A"] = ([Dim.X, Dim.Y], arr1)
        s1 = d1[Dim.X, 2:]
        s2 = copy.copy(s1)
        s3 = copy.deepcopy(s2)
        self.assertEqual(s1, s2)
        self.assertEqual(s3, s2)
        s2 *= s2
        self.assertNotEqual(s1[Data.Value, "A"], s2[Data.Value, "A"])
        self.assertNotEqual(s3[Data.Value, "A"], s2[Data.Value, "A"])

    def _apply_test_op(self, op, a, b, data, lh_var_name="a", rh_var_name="b"):
        # Assume numpy operations are correct as comparitor
        op(data,b[Data.Value, rh_var_name].numpy)
        op(a,b)
        np.testing.assert_equal(a[Data.Value, lh_var_name].numpy, data) # Desired nan comparisons

    def test_binary_operations(self):
        d = Dataset()
        d[Coord.X] = ([Dim.X], np.arange(10))
        d[Data.Value, "a"] = ([Dim.X], np.arange(10, dtype='float64'))
        d[Data.Variance, "a"] = ([Dim.X], np.arange(10, dtype='float64'))
        d[Data.Value, "b"] = ([Dim.X], np.arange(10, dtype='float64'))
        d[Data.Variance, "b"] = ([Dim.X], np.arange(10, dtype='float64'))
        a = d.subset["a"]
        b = d.subset["b"]
        data = np.copy(a[Data.Value, "a"].numpy)
        variance = np.copy(a[Data.Variance, "a"].numpy)

        c = a + b
        # Variables "i" and "j" added despite different names
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data + data))
        self.assertTrue(np.array_equal(c[Data.Variance, "a"].numpy, variance + variance))

        c = a - b
        # Variables "a" and "b" subtracted despite different names
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data - data))
        self.assertTrue(np.array_equal(c[Data.Variance, "a"].numpy, variance + variance))

        c = a * b
        # Variables "a" and "b" subtracted despite different names
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data * data))
        self.assertTrue(np.array_equal(c[Data.Variance, "a"].numpy, variance*(data*data)*2))

        c = a / b
        # Variables "a" and "b" subtracted despite different names
        with np.errstate(invalid='ignore'):
            np.testing.assert_equal(c[Data.Value, "a"].numpy, data / data) 
        np.testing.assert_equal(c[Data.Variance, "a"].numpy, variance*(data*data)*2) 

        self._apply_test_op(operator.iadd, a, b, data)
        self._apply_test_op(operator.isub, a, b, data)
        self._apply_test_op(operator.imul, a, b, data)
        self._apply_test_op(operator.itruediv, a, b, data)

    def test_binary_float_operations(self):
        d = Dataset()
        d[Coord.X] = ([Dim.X], np.arange(10))
        d[Data.Value, "a"] = ([Dim.X], np.arange(10, dtype='float64'))
        d[Data.Value, "b"] = ([Dim.X], np.arange(10, dtype='float64'))
        a = d.subset["a"]
        b = d.subset["b"]
        data = np.copy(a[Data.Value, "a"].numpy)

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
        c = 2.0 + a
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data + 2.0))
        c = 2.0 - a
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, 2.0 - data))
        c = 2.0 * a
        self.assertTrue(np.array_equal(c[Data.Value, "a"].numpy, data * 2.0))

        self._apply_test_op(operator.iadd, a, b, data)
        self._apply_test_op(operator.isub, a, b, data)
        self._apply_test_op(operator.imul, a, b, data)
        self._apply_test_op(operator.itruediv, a, b, data)
        

    def test_equal_not_equal(self):
        d = Dataset()
        d[Coord.X] = ([Dim.X], np.arange(10))
        d[Data.Value, "a"] = ([Dim.X], np.arange(10, dtype='float64'))
        d[Data.Value, "b"] = ([Dim.X], np.arange(10, dtype='float64'))
        a = d.subset["a"]
        b = d.subset["b"]
        c = a + b
        data = np.copy(a[Data.Value, "a"].numpy)
        d2 = d[Dim.X, :]
        a2 = d.subset["a"]
        d3 = Dataset()
        d3[Coord.X] = ([Dim.X], np.arange(10))
        d3[Data.Value, "a"] = ([Dim.X], np.arange(1, 11, dtype='float64'))
        a3 = d3.subset["a"]
        # Equal
        self.assertEqual(d, d2)
        self.assertEqual(d2, d)
        self.assertEqual(a, a2)
        # Not equal
        self.assertNotEqual(a, b)
        self.assertNotEqual(b, a)
        self.assertNotEqual(a, c)
        self.assertNotEqual(a, a3)

    def test_correct_temporaries(self):
        N = 6
        M = 4
        d1 = Dataset()
        d1[Coord.X] = ([Dim.X], np.arange(N+1).astype(np.float64))
        d1[Coord.Y] = ([Dim.Y], np.arange(M+1).astype(np.float64))
        arr1 = np.arange(N*M).reshape(N,M).astype(np.float64) + 1
        d1[Data.Value, "A"] = ([Dim.X, Dim.Y], arr1)
        d1 = d1[Dim.X, 1:2]
        self.assertEqual(list(d1[Data.Value, "A"].data), [5.0, 6.0, 7.0, 8.0])

        
if __name__ == '__main__':
    unittest.main()
