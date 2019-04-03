# @file
# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
import unittest

from scippy import *
import numpy as np

class TestDTypes(unittest.TestCase):
    def test_Eigen_Vector3d(self):
        d = Dataset()
        d[Coord.Position] = ([Dim.Position], (4,))
        self.assertEqual(len(d[Coord.Position].data), 4)
        self.assertEqual(len(d[Coord.Position].data[0]), 3)
        self.assertEqual(d[Coord.Position].data[0][0], 0.0)
        d[Coord.Position].data[0][1] = 1.0
        self.assertEqual(d[Coord.Position].data[0][1], 1.0)

if __name__ == '__main__':
    unittest.main()
