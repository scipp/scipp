# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import unittest

import scippy as sp


class TestDTypes(unittest.TestCase):
    @unittest.skip("Tag-derived dtype not available anymore, need to implement way of specifying Eigen types.")
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
