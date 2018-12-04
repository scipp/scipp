import unittest

from dataset import *
import numpy as np

class TestDataset(unittest.TestCase):
    def setUp(self):
        lx = 2
        ly = 3
        lz = 4
        self.reference_x = np.arange(lx)
        self.reference_y = np.arange(ly)
        self.reference_z = np.arange(lz)
        self.reference_data1 = np.arange(lx*ly*lz).reshape(lz,ly,lx)
        self.reference_data2 = np.ones(lx*ly*lz).reshape(lz,ly,lx)
        self.reference_data3 = np.arange(lx*lz).reshape(lz,lx)

        self.dataset = Dataset()
        self.dataset[Data.Value, "data1"] = ([Dim.Z, Dim.Y, Dim.X], self.reference_data1)
        self.dataset[Data.Value, "data2"] = ([Dim.Z, Dim.Y, Dim.X], self.reference_data2)
        self.dataset[Data.Value, "data3"] = ([Dim.Z, Dim.X], self.reference_data3)
        self.dataset[Coord.X] = ([Dim.X], self.reference_x)
        self.dataset[Coord.Y] = ([Dim.Y], self.reference_y)
        self.dataset[Coord.Z] = ([Dim.Z], self.reference_z)

    def test_size(self):
        # X, Y, Z, 3 x Data::Value
        self.assertEqual(len(self.dataset), 6)

    def test_contains(self):
        self.assertTrue(Coord.X in self.dataset)
        self.assertTrue(Coord.Y in self.dataset)
        self.assertTrue(Coord.Z in self.dataset)
        self.assertTrue((Data.Value, "data1") in self.dataset)
        self.assertTrue((Data.Value, "data2") in self.dataset)
        self.assertTrue((Data.Value, "data3") in self.dataset)
        self.assertFalse((Data.Value, "data4") in self.dataset)

    def test_view_contains(self):
        view = self.dataset["data2"]
        self.assertTrue(Coord.X in view)
        self.assertTrue(Coord.Y in view)
        self.assertTrue(Coord.Z in view)
        self.assertFalse((Data.Value, "data1") in view)
        self.assertTrue((Data.Value, "data2") in view)
        self.assertFalse((Data.Value, "data3") in view)
        self.assertFalse((Data.Value, "data4") in view)

    def test_delitem(self):
        dataset = Dataset()
        dataset[Data.Value, "data"] = ([Dim.Z, Dim.Y, Dim.X], (1,2,3))
        self.assertTrue((Data.Value, "data") in dataset)
        self.assertEqual(len(dataset.dimensions()), 3)
        del dataset[Data.Value, "data"]
        self.assertFalse((Data.Value, "data") in dataset)
        self.assertEqual(len(dataset.dimensions()), 0)

        dataset[Data.Value, "data"] = ([Dim.Z, Dim.Y, Dim.X], (1,2,3))
        dataset[Coord.X] = ([Dim.X], np.arange(3))
        del dataset[Data.Value, "data"]
        self.assertFalse((Data.Value, "data") in dataset)
        self.assertEqual(len(dataset.dimensions()), 1)
        del dataset[Coord.X]
        self.assertFalse(Coord.X in dataset)
        self.assertEqual(len(dataset.dimensions()), 0)

    def test_insert_default_init(self):
        d = Dataset()
        d[Data.Value, "data1"] = ((Dim.Z, Dim.Y, Dim.X), (4,3,2))
        self.assertEqual(len(d), 1)
        np.testing.assert_array_equal(d[Data.Value, "data1"].numpy, np.zeros(shape=(4,3,2)))

    def test_insert(self):
        d = Dataset()
        d[Data.Value, "data1"] = ([Dim.Z, Dim.Y, Dim.X], np.arange(24).reshape(4,3,2))
        self.assertEqual(len(d), 1)
        np.testing.assert_array_equal(d[Data.Value, "data1"].numpy, self.reference_data1)

        # Currently implicitly replacing keys in Dataset is not supported. Should it?
        self.assertRaisesRegex(RuntimeError, "Attempt to insert data of same type with duplicate name.",
                d.__setitem__, (Data.Value, "data1"), ([Dim.Z, Dim.Y, Dim.X], np.arange(24).reshape(4,3,2)))

        self.assertRaisesRegex(RuntimeError, "Cannot insert variable into Dataset: Dimensions do not match.",
                d.__setitem__, (Data.Value, "data2"), ([Dim.Z, Dim.Y, Dim.X], np.arange(24).reshape(4,2,3)))

    def test_dimensions(self):
        self.assertEqual(self.dataset.dimensions().size(Dim.X), 2)
        self.assertEqual(self.dataset.dimensions().size(Dim.Y), 3)
        self.assertEqual(self.dataset.dimensions().size(Dim.Z), 4)

    def test_data(self):
        self.assertEqual(len(self.dataset[Coord.X].data), 2)
        self.assertSequenceEqual(self.dataset[Coord.X].data, [0,1])
        # `data` property provides a flat view
        self.assertEqual(len(self.dataset[Data.Value, "data1"].data), 24)
        self.assertSequenceEqual(self.dataset[Data.Value, "data1"].data, range(24))

    def test_numpy_data(self):
        np.testing.assert_array_equal(self.dataset[Coord.X].numpy, self.reference_x)
        np.testing.assert_array_equal(self.dataset[Coord.Y].numpy, self.reference_y)
        np.testing.assert_array_equal(self.dataset[Coord.Z].numpy, self.reference_z)
        np.testing.assert_array_equal(self.dataset[Data.Value, "data1"].numpy, self.reference_data1)
        np.testing.assert_array_equal(self.dataset[Data.Value, "data2"].numpy, self.reference_data2)
        np.testing.assert_array_equal(self.dataset[Data.Value, "data3"].numpy, self.reference_data3)

    def test_view_subdata(self):
        view = self.dataset["data1"]
        # TODO Need consistent dimensions() implementation for Dataset and its views.
        #self.assertEqual(view.dimensions().size(Dim.X), 2)
        #self.assertEqual(view.dimensions().size(Dim.Y), 3)
        #self.assertEqual(view.dimensions().size(Dim.Z), 4)
        self.assertEqual(len(view), 4)

    def test_slice_dataset(self):
        for x in range(2):
            view = self.dataset[Dim.X, x]
            self.assertRaisesRegex(RuntimeError, 'Dataset does not contain such a variable.', view.__getitem__, Coord.X)
            np.testing.assert_array_equal(view[Coord.Y].numpy, self.reference_y)
            np.testing.assert_array_equal(view[Coord.Z].numpy, self.reference_z)
            np.testing.assert_array_equal(view[Data.Value, "data1"].numpy, self.reference_data1[:,:,x])
            np.testing.assert_array_equal(view[Data.Value, "data2"].numpy, self.reference_data2[:,:,x])
            np.testing.assert_array_equal(view[Data.Value, "data3"].numpy, self.reference_data3[:,x])
        for y in range(3):
            view = self.dataset[Dim.Y, y]
            np.testing.assert_array_equal(view[Coord.X].numpy, self.reference_x)
            self.assertRaisesRegex(RuntimeError, 'Dataset does not contain such a variable.', view.__getitem__, Coord.Y)
            np.testing.assert_array_equal(view[Coord.Z].numpy, self.reference_z)
            np.testing.assert_array_equal(view[Data.Value, "data1"].numpy, self.reference_data1[:,y,:])
            np.testing.assert_array_equal(view[Data.Value, "data2"].numpy, self.reference_data2[:,y,:])
            np.testing.assert_array_equal(view[Data.Value, "data3"].numpy, self.reference_data3)
        for z in range(4):
            view = self.dataset[Dim.Z, z]
            np.testing.assert_array_equal(view[Coord.X].numpy, self.reference_x)
            np.testing.assert_array_equal(view[Coord.Y].numpy, self.reference_y)
            self.assertRaisesRegex(RuntimeError, 'Dataset does not contain such a variable.', view.__getitem__, Coord.Z)
            np.testing.assert_array_equal(view[Data.Value, "data1"].numpy, self.reference_data1[z,:,:])
            np.testing.assert_array_equal(view[Data.Value, "data2"].numpy, self.reference_data2[z,:,:])
            np.testing.assert_array_equal(view[Data.Value, "data3"].numpy, self.reference_data3[z,:])
        for x in range(2):
            for delta in range(3-x):
                view = self.dataset[Dim.X, x:x+delta]
                np.testing.assert_array_equal(view[Coord.X].numpy, self.reference_x[x:x+delta])
                np.testing.assert_array_equal(view[Coord.Y].numpy, self.reference_y)
                np.testing.assert_array_equal(view[Coord.Z].numpy, self.reference_z)
                np.testing.assert_array_equal(view[Data.Value, "data1"].numpy, self.reference_data1[:,:,x:x+delta])
                np.testing.assert_array_equal(view[Data.Value, "data2"].numpy, self.reference_data2[:,:,x:x+delta])
                np.testing.assert_array_equal(view[Data.Value, "data3"].numpy, self.reference_data3[:,x:x+delta])
        for y in range(3):
            for delta in range(4-y):
                view = self.dataset[Dim.Y, y:y+delta]
                np.testing.assert_array_equal(view[Coord.X].numpy, self.reference_x)
                np.testing.assert_array_equal(view[Coord.Y].numpy, self.reference_y[y:y+delta])
                np.testing.assert_array_equal(view[Coord.Z].numpy, self.reference_z)
                np.testing.assert_array_equal(view[Data.Value, "data1"].numpy, self.reference_data1[:,y:y+delta,:])
                np.testing.assert_array_equal(view[Data.Value, "data2"].numpy, self.reference_data2[:,y:y+delta,:])
                np.testing.assert_array_equal(view[Data.Value, "data3"].numpy, self.reference_data3)
        for z in range(4):
            for delta in range(5-z):
                view = self.dataset[Dim.Z, z:z+delta]
                np.testing.assert_array_equal(view[Coord.X].numpy, self.reference_x)
                np.testing.assert_array_equal(view[Coord.Y].numpy, self.reference_y)
                np.testing.assert_array_equal(view[Coord.Z].numpy, self.reference_z[z:z+delta])
                np.testing.assert_array_equal(view[Data.Value, "data1"].numpy, self.reference_data1[z:z+delta,:,:])
                np.testing.assert_array_equal(view[Data.Value, "data2"].numpy, self.reference_data2[z:z+delta,:,:])
                np.testing.assert_array_equal(view[Data.Value, "data3"].numpy, self.reference_data3[z:z+delta,:])

    def test_numpy_interoperable(self):
        # TODO: Need also __setitem__ with view.
        # self.dataset[Data.Value, 'data2'] = self.dataset[Data.Value, 'data1']
        self.dataset[Data.Value, 'data2'] = np.exp(self.dataset[Data.Value, 'data1'])
        np.testing.assert_array_equal(self.dataset[Data.Value, "data2"].numpy, np.exp(self.reference_data1))
        # Restore original value.
        self.dataset[Data.Value, 'data2'] = self.reference_data2
        np.testing.assert_array_equal(self.dataset[Data.Value, "data2"].numpy, self.reference_data2)

    def test_slice_numpy_interoperable(self):
        # Dataset subset then view single variable
        self.dataset['data2'][Data.Value, 'data2'] = np.exp(self.dataset[Data.Value, 'data1'])
        np.testing.assert_array_equal(self.dataset[Data.Value, "data2"].numpy, np.exp(self.reference_data1))
        # Slice view of dataset then view single variable
        self.dataset[Dim.X, 0][Data.Value, 'data2'] = np.exp(self.dataset[Dim.X, 1][Data.Value, 'data1'])
        np.testing.assert_array_equal(self.dataset[Data.Value, "data2"].numpy[...,0], np.exp(self.reference_data1[...,1]))
        # View single variable then slice view
        self.dataset[Data.Value, 'data2'][Dim.X, 1] = np.exp(self.dataset[Data.Value, 'data1'][Dim.X, 0])
        np.testing.assert_array_equal(self.dataset[Data.Value, "data2"].numpy[...,1], np.exp(self.reference_data1[...,0]))
        # View single variable then view range of slices
        self.dataset[Data.Value, 'data2'][Dim.Y, 1:3] = np.exp(self.dataset[Data.Value, 'data1'][Dim.Y, 0:2])
        np.testing.assert_array_equal(self.dataset[Data.Value, "data2"].numpy[:,1:3,:], np.exp(self.reference_data1[:,0:2,:]))

        # Restore original value.
        self.dataset[Data.Value, 'data2'] = self.reference_data2
        np.testing.assert_array_equal(self.dataset[Data.Value, "data2"].numpy, self.reference_data2)

    def test_split(self):
        dataset = Dataset()
        dataset[Data.Value, "data"] = ([Dim.X], np.arange(4))
        dataset[Coord.X] = ([Dim.X], np.array([3,2,4,1]))
        datasets = split(dataset, Dim.X, [2])
        self.assertEqual(len(datasets), 2)
        d0 = datasets[0]
        np.testing.assert_array_equal(d0[Coord.X].numpy, np.array([3,2]))
        np.testing.assert_array_equal(d0[Data.Value, "data"].numpy, np.array([0,1]))
        d1 = datasets[1]
        np.testing.assert_array_equal(d1[Coord.X].numpy, np.array([4,1]))
        np.testing.assert_array_equal(d1[Data.Value, "data"].numpy, np.array([2,3]))

    def test_concatenate(self):
        dataset = Dataset()
        dataset[Data.Value, "data"] = ([Dim.X], np.arange(4))
        dataset[Coord.X] = ([Dim.X], np.array([3,2,4,1]))
        dataset = concatenate(dataset, dataset, Dim.X)
        np.testing.assert_array_equal(dataset[Coord.X].numpy, np.array([3,2,4,1, 3,2,4,1]))
        np.testing.assert_array_equal(dataset[Data.Value, "data"].numpy, np.array([0,1,2,3, 0,1,2,3]))

    def test_concatenate_with_slice(self):
        dataset = Dataset()
        dataset[Data.Value, "data"] = ([Dim.X], np.arange(4))
        dataset[Coord.X] = ([Dim.X], np.array([3,2,4,1]))
        dataset = concatenate(dataset, dataset[Dim.X, 0:2], Dim.X)
        np.testing.assert_array_equal(dataset[Coord.X].numpy, np.array([3,2,4,1, 3,2]))
        np.testing.assert_array_equal(dataset[Data.Value, "data"].numpy, np.array([0,1,2,3, 0,1]))

    def test_rebin(self):
        dataset = Dataset()
        dataset[Data.Value, "data"] = ([Dim.X], np.array(10*[1]))
        dataset[Coord.X] = ([Dim.X], np.arange(11))
        new_coord = Variable(Coord.X, [Dim.X], np.arange(0, 11, 2))
        dataset = rebin(dataset, new_coord)
        np.testing.assert_array_equal(dataset[Data.Value, "data"].numpy, np.array(5*[2]))
        np.testing.assert_array_equal(dataset[Coord.X].numpy, np.arange(0, 11, 2))

    def test_sort(self):
        dataset = Dataset()
        dataset[Data.Value, "data"] = ([Dim.X], np.arange(4))
        dataset[Data.Value, "data2"] = ([Dim.X], np.arange(4))
        dataset[Coord.X] = ([Dim.X], np.array([3,2,4,1]))
        dataset = sort(dataset, Coord.X)
        np.testing.assert_array_equal(dataset[Data.Value, "data"].numpy, np.array([3,1,0,2]))
        np.testing.assert_array_equal(dataset[Coord.X].numpy, np.array([1,2,3,4]))
        dataset = sort(dataset, Data.Value, "data")
        np.testing.assert_array_equal(dataset[Data.Value, "data"].numpy, np.arange(4))
        np.testing.assert_array_equal(dataset[Coord.X].numpy, np.array([3,2,4,1]))

    def test_filter(self):
        dataset = Dataset()
        dataset[Data.Value, "data"] = ([Dim.X], np.arange(4))
        dataset[Coord.X] = ([Dim.X], np.array([3,2,4,1]))
        select = Variable(Coord.Mask, [Dim.X], np.array([False, True, False, True]))
        dataset = filter(dataset, select)
        np.testing.assert_array_equal(dataset[Data.Value, "data"].numpy, np.array([1,3]))
        np.testing.assert_array_equal(dataset[Coord.X].numpy, np.array([2,1]))

class TestDatasetExamples(unittest.TestCase):
    def test_table_example(self):
        table = Dataset()
        table[Coord.RowLabel] = ([Dim.Row], ['a', 'bb', 'ccc', 'dddd'])
        self.assertSequenceEqual(table[Coord.RowLabel].data, ['a', 'bb', 'ccc', 'dddd'])
        table[Data.Value, "col1"] = ([Dim.Row], [3,2,1,0])
        table[Data.Value, "col2"] = ([Dim.Row], np.arange(4))
        self.assertEqual(len(table), 3)

        table = concatenate(table, table, Dim.Row)
        np.testing.assert_array_equal(table[Data.Value, "col1"].numpy, np.array([3,2,1,0, 3,2,1,0]))

        table = concatenate(table[Dim.Row, 0:2], table[Dim.Row, 5:7], Dim.Row)
        np.testing.assert_array_equal(table[Data.Value, "col1"].numpy, np.array([3,2,2,1]))

        table = sort(table, Data.Value, "col1")
        np.testing.assert_array_equal(table[Data.Value, "col1"].numpy, np.array([1,2,2,3]))

        table = sort(table, Coord.RowLabel)
        np.testing.assert_array_equal(table[Data.Value, "col1"].numpy, np.array([3,2,2,1]))

if __name__ == '__main__':
    unittest.main()
