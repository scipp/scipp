import unittest

from dataset import *
import numpy as np

class TestDatasetSlice(unittest.TestCase):

    def setUp(self):
        d = Dataset()
        d[Coord.X] = ([Dim.X], np.arange(10))
        d[Data.Value, "a"] = ([Dim.X], np.arange(10)) 
        d[Data.Value, "b"] = ([Dim.X], np.arange(10)) 
        self._d = d
    
    def test_type(self):
        ds_slice = self._d.subset("a")
        self.assertEqual(type(ds_slice), dataset.DatasetSlice)

    def test_extract_slice(self):
        ds_slice = self._d.subset("a")
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

if __name__ == '__main__':
    unittest.main()
