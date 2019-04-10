# @file
# SPDX-License-Identifier: GPL-3.0-or-later
# @author Simon Heybrock
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
# National Laboratory, and European Spallation Source ERIC.
import unittest

import scippy as sp


class TestDTypes(unittest.TestCase):
    def test_Eigen_Vector3d(self):
        d = sp.Dataset()
        d[sp.Coord.Position] = ([sp.Dim.Position], (4,))
        self.assertEqual(len(d[sp.Coord.Position].data), 4)
        self.assertEqual(len(d[sp.Coord.Position].data[0]), 3)
        self.assertEqual(d[sp.Coord.Position].data[0][0], 0.0)
        d[sp.Coord.Position].data[0][1] = 1.0
        self.assertEqual(d[sp.Coord.Position].data[0][1], 1.0)


if __name__ == '__main__':
    unittest.main()
