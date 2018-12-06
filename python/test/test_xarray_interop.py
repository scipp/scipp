import unittest

import dataset as ds
import numpy as np
import matplotlib.pyplot as plt

class TestXarrayInterop(unittest.TestCase):
    def test_table(self):
        table = ds.Dataset()
        table[ds.Coord.RowLabel] = ([ds.Dim.Row], ['a', 'bb', 'ccc', 'dddd'])
        self.assertSequenceEqual(table[ds.Coord.RowLabel].data, ['a', 'bb', 'ccc', 'dddd'])
        table[ds.Data.Value, "col1"] = ([ds.Dim.Row], [3,2,1,0])
        table[ds.Data.Value, "col2"] = ([ds.Dim.Row], np.arange(4))
        self.assertEqual(len(table), 3)

        table = ds.concatenate(table, table, ds.Dim.Row)
        np.testing.assert_array_equal(table[ds.Data.Value, "col1"].numpy, np.array([3,2,1,0, 3,2,1,0]))

        table = ds.concatenate(table[ds.Dim.Row, 0:2], table[ds.Dim.Row, 5:7], ds.Dim.Row)
        np.testing.assert_array_equal(table[ds.Data.Value, "col1"].numpy, np.array([3,2,2,1]))

        table = ds.sort(table, ds.Data.Value, "col1")
        np.testing.assert_array_equal(table[ds.Data.Value, "col1"].numpy, np.array([1,2,2,3]))

        table = ds.sort(table, ds.Coord.RowLabel)
        np.testing.assert_array_equal(table[ds.Data.Value, "col1"].numpy, np.array([3,2,2,1]))

        for i in range(1, len(table[ds.Coord.RowLabel])):
            table[ds.Dim.Row, i] += table[ds.Dim.Row, i-1]

        np.testing.assert_array_equal(table[ds.Data.Value, "col1"].numpy, np.array([3,5,7,8]))

        table[ds.Data.Value, "exp1"] = ([ds.Dim.Row], np.exp(table[ds.Data.Value, "col1"]))
        table[ds.Data.Value, "exp1"] -= table[ds.Data.Value, "col1"]
        np.testing.assert_array_equal(table[ds.Data.Value, "exp1"].numpy, np.exp(np.array([3,5,7,8]))-np.array([3,5,7,8]))

        table += table
        self.assertSequenceEqual(table[ds.Coord.RowLabel].data, ['a', 'bb', 'bb', 'ccc'])

        dataset = ds.as_xarray(table['col1'])
        #print(dataset)
        dataset['Value:col1'].plot()
        #plt.savefig('test.png')

if __name__ == '__main__':
    unittest.main()
