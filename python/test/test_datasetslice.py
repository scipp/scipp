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
        self.assertEqual(type(ds_slice), dataset.DatasetView)

    def test_extract_slice(self):
        ds_slice = self._d.subset("a")
        self.assertEqual(type(ds_slice), dataset.DatasetView)
        # We should have just one data variable
        self.assertEqual(1, len([var for var in ds_slice if var.is_data]))
        # We should have just one coord variable
        self.assertEqual(1, len([var for var in ds_slice if var.is_coord]))
        self.assertEqual(2, len(ds_slice))


    def test_range_based_slice(self):
        subset = slice(1,4,1)
        # Create slice
        ds_slice = self._d[Dim.X,subset]
        # Test via variable_slice
        self.assertEquals(len(ds_slice[Coord.X]), len(range(subset.start, subset.stop, subset.step)))

    def test_repr(self):
        ds_slice = self._d.subset("a")
        self.assertEqual(repr(ds_slice), "<DatasetSlice>\nDimensions: {{Dim.X, 10}}\nCoordinates:\n    (Coord.X)                 int64     [m]              (Dim.X)\nData:\n    (Data.Value, a)           int64     [dimensionless]  (Dim.X)\nAttributes:\n\n")

if __name__ == '__main__':
    unittest.main()
