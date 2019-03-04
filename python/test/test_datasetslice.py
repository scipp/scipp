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

    def test_extract_slice(self):
        ds_slice = self._d.subset("a")
        self.assertEqual(type(ds_slice), dataset.DatasetView)
        # We should have just one data variable
        self.assertEqual(1, len([var for var in ds_slice if var.is_data]))
        # We should have just one coord variable
        self.assertEqual(1, len([var for var in ds_slice if var.is_coord]))
        self.assertEqual(2, len(ds_slice))

    def test_slice_back_ommit_range(self):
        sl = d[ds.Dim.X, 1:-1][ds.Data.Value, "a"].numpy
        ref = np. array([1, 2, 3, 4, 5, 6, 7, 8], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.self.assertEqual(np.allclose(sl.numpy, ref), True)
        # omitting range end
        sl = d[ds.Dim.X, 1:][ds.Data.Value, "b"].numpy
        ref = np. array([1, 2, 3, 4, 5, 6, 7, 8, 9], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.self.assertEqual(np.allclose(sl.numpy, ref), True)
        # omitting range begin
        sl = d[ds.Dim.X, :-1][ds.Data.Value, "a"].numpy
        ref = np. array([0, 1, 2, 3, 4, 5, 6, 7, 8], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.self.assertEqual(np.allclose(sl.numpy, ref), True)
        # omitting range both begin and end
        sl = d[ds.Dim.X, :][ds.Data.Value, "b"].numpy
        ref = np. array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.self.assertEqual(np.allclose(sl.numpy, ref), True)


if __name__ == '__main__':
    unittest.main()
