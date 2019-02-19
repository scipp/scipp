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
        ds_slice = self._d["a"]
        self.assertEqual(type(ds_slice), dataset.DatasetView)
        # We should have just one data variable
        self.assertEqual(1, len([var for var in ds_slice if var.is_data]))
        # We should have just one coord variable
        self.assertEqual(1, len([var for var in ds_slice if var.is_coord]))
        self.assertEqual(2, len(ds_slice))

    def test_repr(self):
        ds_slice = self._d["a"]
        self.assertEqual(repr(ds_slice), "Dataset slice with 2 variables")


if __name__ == '__main__':
    unittest.main()
