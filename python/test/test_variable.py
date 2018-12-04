import unittest

from dataset import *
import numpy as np

class TestVariable(unittest.TestCase):
    def test_create_coord(self):
        var = Variable(Coord.X, [Dim.X], np.arange(4))
        self.assertEqual(var.name, "")

    def test_create_data(self):
        var = Variable(Data.Value, [Dim.X], np.arange(4))
        self.assertEqual(var.name, "")

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
