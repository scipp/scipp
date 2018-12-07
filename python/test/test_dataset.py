import unittest

from dataset import *
import numpy as np
import matplotlib.pyplot as plt

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
        dataset[Data.Value, "aux"] = ([], ())
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

        table[Data.Value, "sum"] = ([Dim.Row], (len(table[Coord.RowLabel]),))

        for col in table:
            if not col.is_coord and col.name is not "sum":
                table[Data.Value, "sum"] += col
        np.testing.assert_array_equal(table[Data.Value, "col2"].numpy, np.array([0,1,2,3]))
        np.testing.assert_array_equal(table[Data.Value, "sum"].numpy, np.array([6,6,6,6]))

        table = concatenate(table, table, Dim.Row)
        np.testing.assert_array_equal(table[Data.Value, "col1"].numpy, np.array([3,2,1,0, 3,2,1,0]))

        table = concatenate(table[Dim.Row, 0:2], table[Dim.Row, 5:7], Dim.Row)
        np.testing.assert_array_equal(table[Data.Value, "col1"].numpy, np.array([3,2,2,1]))

        table = sort(table, Data.Value, "col1")
        np.testing.assert_array_equal(table[Data.Value, "col1"].numpy, np.array([1,2,2,3]))

        table = sort(table, Coord.RowLabel)
        np.testing.assert_array_equal(table[Data.Value, "col1"].numpy, np.array([3,2,2,1]))

        for i in range(1, len(table[Coord.RowLabel])):
            table[Dim.Row, i] += table[Dim.Row, i-1]

        np.testing.assert_array_equal(table[Data.Value, "col1"].numpy, np.array([3,5,7,8]))

        table[Data.Value, "exp1"] = ([Dim.Row], np.exp(table[Data.Value, "col1"]))
        table[Data.Value, "exp1"] -= table[Data.Value, "col1"]
        np.testing.assert_array_equal(table[Data.Value, "exp1"].numpy, np.exp(np.array([3,5,7,8]))-np.array([3,5,7,8]))

        table += table
        self.assertSequenceEqual(table[Coord.RowLabel].data, ['a', 'bb', 'bb', 'ccc'])

    def test_table_example_no_assert(self):
        table = Dataset()

        # Add columns
        table[Coord.RowLabel] = ([Dim.Row], ['a', 'bb', 'ccc', 'dddd'])
        table[Data.Value, "col1"] = ([Dim.Row], [3,2,1,0])
        table[Data.Value, "col2"] = ([Dim.Row], np.arange(4))
        table[Data.Value, "sum"] = ([Dim.Row], (4,))

        # Do something for each column (here: sum)
        for col in table:
            if not col.is_coord and col.name is not "sum":
                table[Data.Value, "sum"] += col

        # Append tables (append rows of second table to first)
        table = concatenate(table, table, Dim.Row)

        # Append tables sections (e.g., to remove rows from the middle)
        table = concatenate(table[Dim.Row, 0:2], table[Dim.Row, 5:7], Dim.Row)

        # Sort by column
        table = sort(table, Data.Value, "col1")
        # ... or another one
        table = sort(table, Coord.RowLabel)

        # Do something for each row (here: cumulative sum)
        for i in range(1, len(table[Coord.RowLabel])):
            table[Dim.Row, i] += table[Dim.Row, i-1]

        # Apply numpy function to column, store result as a new column
        table[Data.Value, "exp1"] = ([Dim.Row], np.exp(table[Data.Value, "col1"]))
        # ... or as an existing column
        table[Data.Value, "exp1"] = np.sin(table[Data.Value, "exp1"])

        # Remove column
        del table[Data.Value, "exp1"]

        # Arithmetics with tables (here: add two tables)
        table += table

    def test_MDHistoWorkspace_plotting_example(self):
        d = Dataset()
        L = 30
        d[Coord.X] = ([Dim.X], np.arange(L))
        d[Coord.Y] = ([Dim.Y], np.arange(L))
        d[Coord.Z] = ([Dim.Z], np.arange(L))
        d[Data.Value, "temperature"] = ([Dim.Z, Dim.Y, Dim.X], np.random.normal(size=L*L*L).reshape([L,L,L]))

        dataset = as_xarray(d['temperature'])
        dataset['Value:temperature'][10, ...].plot()
        #plt.savefig('test.png')

    def test_MDHistoWorkspace_example(self):
        L = 30
        d = Dataset()

        # Add bin-edge axis for X
        d[Coord.X] = ([Dim.X], np.arange(L+1))
        # ... and normal axes for Y and Z
        d[Coord.Y] = ([Dim.Y], np.arange(L))
        d[Coord.Z] = ([Dim.Z], np.arange(L))

        # Add data variables
        d[Data.Value, "temperature"] = ([Dim.Z, Dim.Y, Dim.X], np.random.normal(size=L*L*L).reshape([L,L,L]))
        d[Data.Value, "pressure"] = ([Dim.Z, Dim.Y, Dim.X], np.random.normal(size=L*L*L).reshape([L,L,L]))
        # Add uncertainties, matching name implicitly links it to corresponding data
        d[Data.Variance, "temperature"] = d[Data.Value, "temperature"]

        # Uncertainties are propagated using grouping mechanism based on name
        square = d * d

        # Rebin the X axis
        d = rebin(d, Variable(Coord.X, [Dim.X], np.arange(0, L+1, 2)))
        # Rebin to different axis for every y
        rebinned = rebin(d, Variable(Coord.X, [Dim.Y, Dim.X], np.arange(0, 2*L).reshape([L,2])))

        # Do something with numpy and insert result
        d[Data.Value, "dz(p)"] = ([Dim.Z, Dim.Y, Dim.X], np.gradient(d[Data.Value, "pressure"], d[Coord.Z], axis=0))

        # Truncate Y and Z axes
        d = Dataset(d[Dim.Y, 10:20][Dim.Z, 10:20])

        # Mean along Y axis
        meanY = mean(d, Dim.Y)
        # Subtract from original, making use of automatic broadcasting
        d -= meanY

        # Extract a Z slice
        sliceZ = Dataset(d[Dim.Z, 7])

    def test_Workspace2D_example(self):
        d = Dataset()

        d[Coord.SpectrumNumber] = ([Dim.Spectrum], np.arange(1, 101))

        # Add a (common) time-of-flight axis
        d[Coord.Tof] = ([Dim.Tof], np.arange(1000))

        # Add data with uncertainties
        d[Data.Value, "sample1"] = ([Dim.Spectrum, Dim.Tof], np.random.poisson(size=100*1000).reshape([100, 1000]))
        d[Data.Variance, "sample1"] = d[Data.Value, "sample1"]

        # Create a mask and use it to extract some of the spectra
        select = Variable(Coord.Mask, [Dim.Spectrum], np.isin(d[Coord.SpectrumNumber], np.arange(10, 20)))
        spectra = filter(d, select)

        # Direct representation of a simple instrument (more standard Mantid instrument
        # representation is of course supported, this is just to demonstrate the flexibility)
        steps = np.arange(-0.45, 0.46, 0.1)
        x = np.tile(steps,(10,))
        y = x.reshape([10,10]).transpose().flatten()
        d[Coord.X] = ([Dim.Spectrum], x)
        d[Coord.Y] = ([Dim.Spectrum], y)
        d[Coord.Z] = ([], 10.0)

        # Mask some spectra based on distance from beam center
        r = np.sqrt(np.square(d[Coord.X]) + np.square(d[Coord.Y]))
        d[Coord.Mask] = ([Dim.Spectrum], np.less(r, 0.2))

        # Do something for each spectrum (here: apply mask)
        d[Coord.Mask].data
        for i, masked in enumerate(d[Coord.Mask].numpy):
            spec = d[Dim.Spectrum, i]
            if masked:
                spec[Data.Value, "sample1"] = np.zeros(1000)
                spec[Data.Variance, "sample1"] = np.zeros(1000)


if __name__ == '__main__':
    unittest.main()
