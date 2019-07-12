# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import unittest

import scippy as sp
from scippy.xarray_compat import as_xarray
import numpy as np


class TestXarrayInterop(unittest.TestCase):
    def test_table(self):
        table = sp.Dataset()
        table[sp.Coord.Row] = ([sp.Dim.Row], ['a', 'bb', 'ccc', 'dddd'])
        self.assertSequenceEqual(table[sp.Coord.Row].data, [
                                 'a', 'bb', 'ccc', 'dddd'])
        table[sp.Data.Value, "col1"] = ([sp.Dim.Row], [3.0, 2.0, 1.0, 0.0])
        table[sp.Data.Value, "col2"] = ([sp.Dim.Row], np.arange(4))
        self.assertEqual(len(table), 3)

        table = sp.concatenate(table, table, sp.Dim.Row)
        np.testing.assert_array_equal(table[sp.Data.Value, "col1"].numpy,
                                      np.array([3, 2, 1, 0, 3, 2, 1, 0]))

        table = sp.concatenate(table[sp.Dim.Row, 0:2], table[sp.Dim.Row, 5:7],
                               sp.Dim.Row)
        np.testing.assert_array_equal(table[sp.Data.Value, "col1"].numpy,
                                      np.array([3, 2, 2, 1]))

        table = sp.sort(table, sp.Data.Value, "col1")
        np.testing.assert_array_equal(table[sp.Data.Value, "col1"].numpy,
                                      np.array([1, 2, 2, 3]))

        table = sp.sort(table, sp.Coord.Row)
        np.testing.assert_array_equal(table[sp.Data.Value, "col1"].numpy,
                                      np.array([3, 2, 2, 1]))

        for i in range(1, len(table[sp.Coord.Row])):
            table[sp.Dim.Row, i] += table[sp.Dim.Row, i - 1]

        np.testing.assert_array_equal(table[sp.Data.Value, "col1"].numpy,
                                      np.array([3, 5, 7, 8]))

        table[sp.Data.Value, "exp1"] = ([sp.Dim.Row],
                                        np.exp(table[sp.Data.Value, "col1"]))
        table[sp.Data.Value, "exp1"] -= table[sp.Data.Value, "col1"]
        np.testing.assert_array_equal(table[sp.Data.Value, "exp1"].numpy,
                                      np.exp(np.array([3, 5, 7, 8])) -
                                      np.array([3, 5, 7, 8]))

        table += table
        self.assertSequenceEqual(table[sp.Coord.Row].data,
                                 ['a', 'bb', 'bb', 'ccc'])

        # xarray cannot deal with non-numeric dimension coordinates
        del(table[sp.Coord.Row])
        dataset = as_xarray(table.subset['col1'])
        # print(dataset)
        dataset['Value:col1'].plot()
        # plt.savefig('test.png')


if __name__ == '__main__':
    unittest.main()
