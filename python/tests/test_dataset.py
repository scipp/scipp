# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import scipp as sp
from scipp import Dim
import numpy as np


def test_create_empty():
    d = sp.Dataset()
    assert len(d) == 0


def test_create():
    x = sp.Variable([Dim.X], values=np.arange(3))
    y = sp.Variable([Dim.Y], values=np.arange(4))
    xy = sp.Variable([Dim.X, Dim.Y], values=np.arange(12).reshape(3, 4))
    d = sp.Dataset({'xy': xy, 'x': x}, coords={Dim.X: x, Dim.Y: y})
    assert len(d) == 2
    assert d.coords[Dim.X] == x
    assert d.coords[Dim.Y] == y
    assert d['xy'].data == xy
    assert d['x'].data == x


def test_setitem():
    d = sp.Dataset()
    d['a'] = sp.Variable(1.0)
    assert len(d) == 1
    assert d['a'].data == sp.Variable(1.0)
    assert len(d.coords) == 0


def test_set_coord():
    d = sp.Dataset()
    d.set_coord(Dim.X, sp.Variable(1.0))
    assert len(d) == 0
    assert len(d.coords) == 1
    assert d.coords[Dim.X] == sp.Variable(1.0)


def test_slice_item():
    d = sp.Dataset()
    d.set_coord(Dim.X, sp.Variable([Dim.X], values=np.arange(4, 8)))
    d['a'] = sp.Variable([Dim.X], values=np.arange(4))
    assert d['a'][Dim.X, 2:4].data == sp.Variable(
        [Dim.X], values=np.arange(2, 4))
    assert d['a'][Dim.X, 2:4].coords[Dim.X] == sp.Variable(
        [Dim.X], values=np.arange(6, 8))


def test_set_item_slice_from_numpy():
    d = sp.Dataset()
    d.set_coord(Dim.X, sp.Variable([Dim.X], values=np.arange(4, 8)))
    d['a'] = sp.Variable([Dim.X], values=np.arange(4))
    d['a'][Dim.X, 2:4] = np.arange(2)
    assert d['a'].data == sp.Variable([Dim.X], values=np.array([0, 1, 0, 1]))


def test_set_item_slice_with_variances_from_numpy():
    d = sp.Dataset()
    d.set_coord(Dim.X, sp.Variable([Dim.X], values=np.arange(4, 8)))
    d['a'] = sp.Variable([Dim.X], values=np.arange(4), variances=np.arange(4))
    d['a'][Dim.X, 2:4].values = np.arange(2)
    d['a'][Dim.X, 2:4].variances = np.arange(2, 4)
    assert np.array_equal(d['a'].values, np.array([0.0, 1.0, 0.0, 1.0]))
    assert np.array_equal(d['a'].variances, np.array([0.0, 1.0, 2.0, 3.0]))


def test_sparse_setitem():
    d = sp.Dataset({'a': sp.Variable([sp.Dim.X, sp.Dim.Y], [4, sp.Dimensions.Sparse])})
    d['a'][Dim.X, 0].values = np.arange(4)
    assert len(d['a'][Dim.X, 0].values) == 4


def test_iadd_slice():
    d = sp.Dataset()
    d.set_coord(Dim.X, sp.Variable([Dim.X], values=np.arange(4, 8)))
    d['a'] = sp.Variable([Dim.X], values=np.arange(4))
    d['a'][Dim.X, 1] += d['a'][Dim.X, 2]
    assert d['a'].data == sp.Variable([Dim.X], values=np.array([0, 3, 2, 3]))


def test_iadd_range():
    d = sp.Dataset()
    d.set_coord(Dim.X, sp.Variable([Dim.X], values=np.arange(4, 8)))
    d['a'] = sp.Variable([Dim.X], values=np.arange(4))
    with pytest.raises(RuntimeError):
        d['a'][Dim.X, 2:4] += d['a'][Dim.X, 1:3]
    d['a'][Dim.X, 2:4] += d['a'][Dim.X, 2:4]
    assert d['a'].data == sp.Variable([Dim.X], values=np.array([0, 1, 4, 6]))


def test_contains():
    d = sp.Dataset()
    assert 'a' not in d
    d['a'] = sp.Variable(1.0)
    assert 'a' in d
    assert 'b' not in d
    d['b'] = sp.Variable(1.0)
    assert 'a' in d
    assert 'b' in d


def test_slice():
    d = sp.Dataset({'a': sp.Variable([Dim.X], values=np.arange(10.0)),
                    'b': sp.Variable(1.0)},
                   coords={
                       Dim.X: sp.Variable([Dim.X], values=np.arange(10.0))})
    expected = sp.Dataset({'a': sp.Variable(1.0)})

    assert d[Dim.X, 1] == expected
    assert 'a' in d[Dim.X, 1]
    assert 'b' not in d[Dim.X, 1]

# def test_delitem(self):
#    dataset = sp.Dataset()
#    dataset[sp.Data.Value, "data"] = ([sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
#                                      (1, 2, 3))
#    dataset[sp.Data.Value, "aux"] = ([], ())
#    self.assertTrue((sp.Data.Value, "data") in dataset)
#    self.assertEqual(len(dataset.dimensions), 3)
#    del dataset[sp.Data.Value, "data"]
#    self.assertFalse((sp.Data.Value, "data") in dataset)
#    self.assertEqual(len(dataset.dimensions), 0)
#
#    dataset[sp.Data.Value, "data"] = ([sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
#                                      (1, 2, 3))
#    dataset[sp.Coord.X] = ([sp.Dim.X], np.arange(3))
#    del dataset[sp.Data.Value, "data"]
#    self.assertFalse((sp.Data.Value, "data") in dataset)
#    self.assertEqual(len(dataset.dimensions), 1)
#    del dataset[sp.Coord.X]
#    self.assertFalse(sp.Coord.X in dataset)
#    self.assertEqual(len(dataset.dimensions), 0)
#
# def test_insert_default_init(self):
#    d = sp.Dataset()
#    d[sp.Data.Value, "data1"] = ((sp.Dim.Z, sp.Dim.Y, sp.Dim.X), (4, 3, 2))
#    self.assertEqual(len(d), 1)
#    np.testing.assert_array_equal(
#        d[sp.Data.Value, "data1"].numpy, np.zeros(shape=(4, 3, 2)))
#
# def test_insert(self):
#    d = sp.Dataset()
#    d[sp.Data.Value, "data1"] = (
#        [sp.Dim.Z, sp.Dim.Y, sp.Dim.X], np.arange(24).reshape(4, 3, 2))
#    self.assertEqual(len(d), 1)
#    np.testing.assert_array_equal(
#        d[sp.Data.Value, "data1"].numpy, self.reference_data1)
#
#    self.assertRaisesRegex(
#        RuntimeError,
#        "Cannot insert variable into Dataset: Dimensions do not match.",
#        d.__setitem__,
#        (sp.Data.Value,
#         "data2"),
#        ([
#            sp.Dim.Z,
#            sp.Dim.Y,
#            sp.Dim.X],
#            np.arange(24).reshape(
#            4,
#            2,
#            3)))
#
# def test_replace(self):
#    d = sp.Dataset()
#    d[sp.Data.Value, "data1"] = (
#        [sp.Dim.Z, sp.Dim.Y, sp.Dim.X], np.zeros(24).reshape(4, 3, 2))
#    d[sp.Data.Value, "data1"] = (
#        [sp.Dim.Z, sp.Dim.Y, sp.Dim.X], np.arange(24).reshape(4, 3, 2))
#    self.assertEqual(len(d), 1)
#    np.testing.assert_array_equal(
#        d[sp.Data.Value, "data1"].numpy, self.reference_data1)
#
# def test_insert_Variable(self):
#    d = sp.Dataset()
#    d[sp.Data.Value, "data1"] = (
#        [sp.Dim.Z, sp.Dim.Y, sp.Dim.X], np.arange(24).reshape(4, 3, 2))
#
#    var = sp.Variable([sp.Dim.X], np.arange(2))
#    d[sp.Data.Value, "data2"] = var
#    d[sp.Data.Variance, "data2"] = var
#    self.assertEqual(len(d), 3)
#
# def test_insert_variable_slice(self):
#    d = sp.Dataset()
#    d[sp.Data.Value, "data1"] = (
#        [sp.Dim.Z, sp.Dim.Y, sp.Dim.X], np.arange(24).reshape(4, 3, 2))
#
#    d[sp.Data.Value, "data2"] = d[sp.Data.Value, "data1"]
#    d[sp.Data.Variance, "data2"] = d[sp.Data.Value, "data1"]
#    self.assertEqual(len(d), 3)
#
# This characterises existing broken behaviour. Will need to be fixed.
# def test_demo_int_to_float_issue(self):
#    # Demo bug
#    d = sp.Dataset()
#    # Variable containing int array data
#    d[sp.Data.Value, "v1"] = ([sp.Dim.X, sp.Dim.Y], np.ndarray.tolist(
#        np.arange(0, 10).reshape(2, 5)))
#    # Correct behaviour should be int64
#    self.assertEqual(d[sp.Data.Value, "v1"].numpy.dtype, 'float64')
#
#    # Demo working 1D
#    d = sp.Dataset()
#    d[sp.Data.Value, "v2"] = ([sp.Dim.X], np.ndarray.tolist(
#        np.arange(0, 10)))  # Variable containing int array data
#    self.assertEqual(d[sp.Data.Value, "v2"].numpy.dtype, 'int64')
#
# def test_set_data(self):
#    d = sp.Dataset()
#    d[sp.Data.Value, "data1"] = (
#        [sp.Dim.Z, sp.Dim.Y, sp.Dim.X], np.arange(24).reshape(4, 3, 2))
#    self.assertEqual(d[sp.Data.Value, "data1"].numpy.dtype, np.int64)
#    d[sp.Data.Value, "data1"] = np.arange(24).reshape(4, 3, 2)
#    self.assertEqual(d[sp.Data.Value, "data1"].numpy.dtype, np.int64)
#    # For existing items we do *not* change the dtype, but convert.
#    d[sp.Data.Value, "data1"] = np.arange(24.0).reshape(4, 3, 2)
#    self.assertEqual(d[sp.Data.Value, "data1"].numpy.dtype, np.int64)
#
# def test_nested_0D_empty_item(self):
#    d = sp.Dataset()
#    d[sp.Data.Events] = ([], sp.Dataset())
#    self.assertEqual(d[sp.Data.Events].data[0], sp.Dataset())
#
# def test_set_data_nested(self):
#    d = sp.Dataset()
#    table = sp.Dataset()
#    table[sp.Data.Value, "col1"] = ([sp.Dim.Row], [3.0, 2.0, 1.0, 0.0])
#    table[sp.Data.Value, "col2"] = ([sp.Dim.Row], np.arange(4.0))
#    d[sp.Data.Value, "data1"] = ([sp.Dim.X], [table, table])
#    d[sp.Data.Value, "data1"].data[1][sp.Data.Value, "col1"].data[0] = 0.0
#    self.assertEqual(d[sp.Data.Value, "data1"].data[0], table)
#    self.assertNotEqual(d[sp.Data.Value, "data1"].data[1], table)
#    self.assertNotEqual(d[sp.Data.Value, "data1"].data[0],
#                        d[sp.Data.Value, "data1"].data[1])
#
# def test_dimensions(self):
#    self.assertEqual(self.dataset.dimensions[sp.Dim.X], 2)
#    self.assertEqual(self.dataset.dimensions[sp.Dim.Y], 3)
#    self.assertEqual(self.dataset.dimensions[sp.Dim.Z], 4)
#
# def test_data(self):
#    self.assertEqual(len(self.dataset[sp.Coord.X].data), 2)
#    self.assertSequenceEqual(self.dataset[sp.Coord.X].data, [0, 1])
#    # `data` property provides a flat view
#    self.assertEqual(len(self.dataset[sp.Data.Value, "data1"].data), 24)
#    self.assertSequenceEqual(
#        self.dataset[sp.Data.Value, "data1"].data, range(24))
#
# def test_numpy_data(self):
#    np.testing.assert_array_equal(
#        self.dataset[sp.Coord.X].numpy, self.reference_x)
#    np.testing.assert_array_equal(
#        self.dataset[sp.Coord.Y].numpy, self.reference_y)
#    np.testing.assert_array_equal(
#        self.dataset[sp.Coord.Z].numpy, self.reference_z)
#    np.testing.assert_array_equal(
#        self.dataset[sp.Data.Value, "data1"].numpy, self.reference_data1)
#    np.testing.assert_array_equal(
#        self.dataset[sp.Data.Value, "data2"].numpy, self.reference_data2)
#    np.testing.assert_array_equal(
#        self.dataset[sp.Data.Value, "data3"].numpy, self.reference_data3)
#
# def test_view_subdata(self):
#    view = self.dataset.subset["data1"]
#    # TODO Need consistent dimensions() implementation for Dataset and its
#    # views.
#    self.assertEqual(view.dimensions[sp.Dim.X], 2)
#    self.assertEqual(view.dimensions[sp.Dim.Y], 3)
#    self.assertEqual(view.dimensions[sp.Dim.Z], 4)
#    self.assertEqual(len(view), 4)
#
# def test_insert_subdata(self):
#    d1 = sp.Dataset()
#    d1[sp.Data.Value, "a"] = ([sp.Dim.X], np.arange(10, dtype="double"))
#    d1[sp.Data.Variance, "a"] = ([sp.Dim.X], np.arange(10, dtype="double"))
#    ds_slice = d1.subset["a"]
#
#    d2 = sp.Dataset()
#    # Insert from subset
#    d2.subset["a"] = ds_slice
#    self.assertEqual(len(d1), len(d2))
#    self.assertEqual(d1, d2)
#
#    d3 = sp.Dataset()
#    # Insert from subset
#    d3.subset["b"] = ds_slice
#    self.assertEqual(len(d3), 2)
#    self.assertNotEqual(d1, d3)  # imported names should differ
#
#    d4 = sp.Dataset()
#    d4.subset["2a"] = ds_slice + ds_slice
#    self.assertEqual(len(d4), 2)
#    self.assertNotEqual(d1, d4)
#    self.assertTrue(np.array_equal(
#        d4[sp.Data.Value, "2a"].numpy,
#        ds_slice[sp.Data.Value, "a"].numpy * 2))
#
# def test_insert_subdata_different_variable_types(self):
#    a = sp.Dataset()
#    xcoord = sp.Variable([sp.Dim.X], np.arange(4))
#    a[sp.Data.Value] = ([sp.Dim.X], np.arange(3))
#    a[sp.Coord.X] = xcoord
#    a[sp.Attr.ExperimentLog] = ([], sp.Dataset())
#
#    b = sp.Dataset()
#    with self.assertRaises(RuntimeError):
#        b.subset["b"] = a[sp.Dim.X, :]  # Coordinates dont match
#    b[sp.Coord.X] = xcoord
#    b.subset["b"] = a[sp.Dim.X, :]  # Should now work
#    self.assertEqual(len(a), len(b))
#    self.assertTrue((sp.Attr.ExperimentLog, "b") in b)
#
# def test_slice_dataset(self):
#    for x in range(2):
#        view = self.dataset[sp.Dim.X, x]
#        self.assertRaisesRegex(
#            RuntimeError,
#            'could not find variable with tag Coord::X and name ``.',
#            view.__getitem__,
#            sp.Coord.X)
#        np.testing.assert_array_equal(
#            view[sp.Coord.Y].numpy, self.reference_y)
#        np.testing.assert_array_equal(
#            view[sp.Coord.Z].numpy, self.reference_z)
#        np.testing.assert_array_equal(
#            view[sp.Data.Value, "data1"].numpy,
#            self.reference_data1[:, :, x])
#        np.testing.assert_array_equal(
#            view[sp.Data.Value, "data2"].numpy,
#            self.reference_data2[:, :, x])
#        np.testing.assert_array_equal(
#            view[sp.Data.Value, "data3"].numpy,
#            self.reference_data3[:, x])
#    for y in range(3):
#        view = self.dataset[sp.Dim.Y, y]
#        np.testing.assert_array_equal(
#            view[sp.Coord.X].numpy, self.reference_x)
#        self.assertRaisesRegex(
#            RuntimeError,
#            'could not find variable with tag Coord::Y and name ``.',
#            view.__getitem__,
#            sp.Coord.Y)
#        np.testing.assert_array_equal(
#            view[sp.Coord.Z].numpy, self.reference_z)
#        np.testing.assert_array_equal(
#            view[sp.Data.Value, "data1"].numpy,
#            self.reference_data1[:, y, :])
#        np.testing.assert_array_equal(
#            view[sp.Data.Value, "data2"].numpy,
#            self.reference_data2[:, y, :])
#        np.testing.assert_array_equal(
#            view[sp.Data.Value, "data3"].numpy,
#            self.reference_data3)
#    for z in range(4):
#        view = self.dataset[sp.Dim.Z, z]
#        np.testing.assert_array_equal(
#            view[sp.Coord.X].numpy, self.reference_x)
#        np.testing.assert_array_equal(
#            view[sp.Coord.Y].numpy, self.reference_y)
#        self.assertRaisesRegex(
#            RuntimeError,
#            'could not find variable with tag Coord::Z and name ``.',
#            view.__getitem__,
#            sp.Coord.Z)
#        np.testing.assert_array_equal(
#            view[sp.Data.Value, "data1"].numpy,
#            self.reference_data1[z, :, :])
#        np.testing.assert_array_equal(
#            view[sp.Data.Value, "data2"].numpy,
#            self.reference_data2[z, :, :])
#        np.testing.assert_array_equal(
#            view[sp.Data.Value, "data3"].numpy, self.reference_data3[z, :])
#    for x in range(2):
#        for delta in range(3 - x):
#            view = self.dataset[sp.Dim.X, x:x + delta]
#            np.testing.assert_array_equal(
#                view[sp.Coord.X].numpy, self.reference_x[x:x + delta])
#            np.testing.assert_array_equal(
#                view[sp.Coord.Y].numpy, self.reference_y)
#            np.testing.assert_array_equal(
#                view[sp.Coord.Z].numpy, self.reference_z)
#            np.testing.assert_array_equal(
#                view[sp.Data.Value, "data1"].numpy,
#                self.reference_data1[:, :, x:x + delta])
#            np.testing.assert_array_equal(
#                view[sp.Data.Value, "data2"].numpy,
#                self.reference_data2[:, :, x:x + delta])
#            np.testing.assert_array_equal(
#                view[sp.Data.Value, "data3"].numpy,
#                self.reference_data3[:, x:x + delta])
#    for y in range(3):
#        for delta in range(4 - y):
#            view = self.dataset[sp.Dim.Y, y:y + delta]
#            np.testing.assert_array_equal(
#                view[sp.Coord.X].numpy, self.reference_x)
#            np.testing.assert_array_equal(
#                view[sp.Coord.Y].numpy, self.reference_y[y:y + delta])
#            np.testing.assert_array_equal(
#                view[sp.Coord.Z].numpy, self.reference_z)
#            np.testing.assert_array_equal(
#                view[sp.Data.Value, "data1"].numpy,
#                self.reference_data1[:, y:y + delta, :])
#            np.testing.assert_array_equal(
#                view[sp.Data.Value, "data2"].numpy,
#                self.reference_data2[:, y:y + delta, :])
#            np.testing.assert_array_equal(
#                view[sp.Data.Value, "data3"].numpy,
#                self.reference_data3)
#    for z in range(4):
#        for delta in range(5 - z):
#            view = self.dataset[sp.Dim.Z, z:z + delta]
#            np.testing.assert_array_equal(
#                view[sp.Coord.X].numpy, self.reference_x)
#            np.testing.assert_array_equal(
#                view[sp.Coord.Y].numpy, self.reference_y)
#            np.testing.assert_array_equal(
#                view[sp.Coord.Z].numpy, self.reference_z[z:z + delta])
#            np.testing.assert_array_equal(
#                view[sp.Data.Value, "data1"].numpy,
#                self.reference_data1[z:z + delta, :, :])
#            np.testing.assert_array_equal(
#                view[sp.Data.Value, "data2"].numpy,
#                self.reference_data2[z:z + delta, :, :])
#            np.testing.assert_array_equal(
#                view[sp.Data.Value, "data3"].numpy,
#                self.reference_data3[z:z + delta, :])
#
# def _apply_test_op_rhs_dataset(
#    self,
#    op,
#    a,
#    b,
#    data,
#    lh_var_name="i",
#        rh_var_name="j"):
#    # Assume numpy operations are correct as comparitor
#    op(data, b[sp.Data.Value, rh_var_name].numpy)
#    op(a, b)
#    # Desired nan comparisons
#    np.testing.assert_equal(a[sp.Data.Value, lh_var_name].numpy, data)
#
# def _apply_test_op_rhs_Variable(
#    self,
#    op,
#    a,
#    b,
#    data,
#    lh_var_name="i",
#        rh_var_name="j"):
#    # Assume numpy operations are correct as comparitor
#    op(data, b.numpy)
#    op(a, b)
#    # Desired nan comparisons
#    np.testing.assert_equal(a[sp.Data.Value, lh_var_name].numpy, data)
#
# def test_binary_dataset_rhs_operations(self):
#    a = sp.Dataset()
#    a[sp.Coord.X] = ([sp.Dim.X], np.arange(10))
#    a[sp.Data.Value, "i"] = ([sp.Dim.X], np.arange(10, dtype='float64'))
#    a[sp.Data.Variance, "i"] = ([sp.Dim.X], np.arange(10, dtype='float64'))
#    b = sp.Dataset()
#    b[sp.Data.Value, "j"] = ([sp.Dim.X], np.arange(10, dtype='float64'))
#    b[sp.Data.Variance, "j"] = ([sp.Dim.X], np.arange(10, dtype='float64'))
#    data = np.copy(a[sp.Data.Value, "i"].numpy)
#    variance = np.copy(a[sp.Data.Variance, "i"].numpy)
#
#    c = a + b
#    # Variables "i" and "j" added despite different names
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data + data))
#    self.assertTrue(np.array_equal(
#        c[sp.Data.Variance, "i"].numpy, variance + variance))
#
#    c = a - b
#    # Variables "a" and "b" subtracted despite different names
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data - data))
#    self.assertTrue(np.array_equal(
#        c[sp.Data.Variance, "i"].numpy, variance + variance))
#
#    c = a * b
#    # Variables "a" and "b" multiplied despite different names
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data * data))
#    self.assertTrue(np.array_equal(
#        c[sp.Data.Variance, "i"].numpy, variance * (data * data) * 2))
#
#    c = a / b
#    # Variables "a" and "b" divided despite different names
#    with np.errstate(invalid="ignore"):
#        np.testing.assert_equal(c[sp.Data.Value, "i"].numpy, data / data)
#        np.testing.assert_equal(c[sp.Data.Variance, "i"].numpy,
#                                2 * variance / (data * data))
#
#    self._apply_test_op_rhs_dataset(operator.iadd, a, b, data)
#    self._apply_test_op_rhs_dataset(operator.isub, a, b, data)
#    self._apply_test_op_rhs_dataset(operator.imul, a, b, data)
#    with np.errstate(invalid="ignore"):
#        self._apply_test_op_rhs_dataset(operator.itruediv, a, b, data)
#
# def test_binary_variable_rhs_operations(self):
#    data = np.ones(10, dtype='float64')
#
#    a = sp.Dataset()
#    a[sp.Coord.X] = ([sp.Dim.X], np.arange(10))
#    a[sp.Data.Value, "i"] = ([sp.Dim.X], data)
#
#    b_var = sp.Variable([sp.Dim.X], data)
#
#    c = a + b_var
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data + data))
#    c = a - b_var
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data - data))
#    c = a * b_var
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data * data))
#    c = a / b_var
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data / data))
#
#    self._apply_test_op_rhs_Variable(operator.iadd, a, b_var, data)
#    self._apply_test_op_rhs_Variable(operator.isub, a, b_var, data)
#    self._apply_test_op_rhs_Variable(operator.imul, a, b_var, data)
#    with np.errstate(invalid="ignore"):
#        self._apply_test_op_rhs_Variable(operator.itruediv, a, b_var, data)
#
# def test_binary_float_operations(self):
#    a = sp.Dataset()
#    a[sp.Coord.X] = ([sp.Dim.X], np.arange(10))
#    a[sp.Data.Value, "i"] = ([sp.Dim.X], np.arange(10, dtype='float64'))
#    b = sp.Dataset()
#    b[sp.Data.Value, "j"] = ([sp.Dim.X], np.arange(10, dtype='float64'))
#    data = np.copy(a[sp.Data.Value, "i"].numpy)
#
#    c = a + 2.0
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data + 2.0))
#    c = a - b
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data - data))
#    c = a - 2.0
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data - 2.0))
#    c = a * 2.0
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data * 2.0))
#    c = a / 2.0
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data / 2.0))
#    c = 2.0 + a
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data + 2.0))
#    c = 2.0 - a
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   2.0 - data))
#    c = 2.0 * a
#    self.assertTrue(np.array_equal(c[sp.Data.Value, "i"].numpy,
#                                   data * 2.0))
#
# def test_equal_not_equal(self):
#    a = sp.Dataset()
#    a[sp.Coord.X] = ([sp.Dim.X], np.arange(10))
#    a[sp.Data.Value, "i"] = ([sp.Dim.X], np.arange(10, dtype='float64'))
#    b = sp.Dataset()
#    b[sp.Data.Value, "j"] = ([sp.Dim.X], np.arange(10, dtype='float64'))
#    c = a + b
#    d = sp.Dataset()
#    d[sp.Coord.X] = ([sp.Dim.X], np.arange(10))
#    d[sp.Data.Value, "i"] = ([sp.Dim.X], np.arange(10, dtype='float64'))
#    a_slice = a[sp.Dim.X, :]
#    d_slice = d[sp.Dim.X, :]
#    # Equal
#    self.assertEqual(a, d)
#    self.assertEqual(a, a_slice)
#    self.assertEqual(a_slice, d_slice)
#    self.assertEqual(d, a)
#    self.assertEqual(d_slice, a)
#    self.assertEqual(d_slice, a_slice)
#    # Not equal
#    self.assertNotEqual(a, b)
#    self.assertNotEqual(a, c)
#    self.assertNotEqual(a_slice, c)
#    self.assertNotEqual(c, a)
#    self.assertNotEqual(c, a_slice)
#
# def test_plus_equals_slice(self):
#    dataset = sp.Dataset()
#    dataset[sp.Data.Value, "data1"] = (
#        [sp.Dim.Z, sp.Dim.Y, sp.Dim.X], self.reference_data1)
#    a = sp.Dataset(dataset[sp.Dim.X, 0])
#    b = dataset[sp.Dim.X, 1]
#    a += b
#
# def test_numpy_interoperable(self):
#    # TODO: Need also __setitem__ with view.
#    # self.dataset[sp.Data.Value, 'data2'] =
#    #     self.dataset[sp.Data.Value, 'data1']
#    self.dataset[sp.Data.Value, 'data2'] = np.exp(
#        self.dataset[sp.Data.Value, 'data1'])
#    np.testing.assert_array_equal(
#        self.dataset[sp.Data.Value, "data2"].numpy,
#        np.exp(self.reference_data1))
#    # Restore original value.
#    self.dataset[sp.Data.Value, 'data2'] = self.reference_data2
#    np.testing.assert_array_equal(
#        self.dataset[sp.Data.Value, "data2"].numpy, self.reference_data2)
#
# def test_slice_numpy_interoperable(self):
#    # Dataset subset then view single variable
#    self.dataset.subset['data2'][sp.Data.Value, 'data2'] = np.exp(
#        self.dataset[sp.Data.Value, 'data1'])
#    np.testing.assert_array_equal(
#        self.dataset[sp.Data.Value, "data2"].numpy,
#        np.exp(self.reference_data1))
#    # Slice view of dataset then view single variable
#    self.dataset[sp.Dim.X, 0][sp.Data.Value, 'data2'] = np.exp(
#        self.dataset[sp.Dim.X, 1][sp.Data.Value, 'data1'])
#    np.testing.assert_array_equal(
#        self.dataset[sp.Data.Value, "data2"].numpy[..., 0],
#        np.exp(self.reference_data1[..., 1]))
#    # View single variable then slice view
#    self.dataset[sp.Data.Value, 'data2'][sp.Dim.X, 1] = np.exp(
#        self.dataset[sp.Data.Value, 'data1'][sp.Dim.X, 0])
#    np.testing.assert_array_equal(
#        self.dataset[sp.Data.Value, "data2"].numpy[..., 1],
#        np.exp(self.reference_data1[..., 0]))
#    # View single variable then view range of slices
#    self.dataset[sp.Data.Value, 'data2'][sp.Dim.Y, 1:3] = np.exp(
#        self.dataset[sp.Data.Value, 'data1'][sp.Dim.Y, 0:2])
#    np.testing.assert_array_equal(self.dataset[sp.Data.Value,
#                                               "data2"].numpy[:, 1:3, :],
#                                  np.exp(self.reference_data1[:, 0:2, :]))
#
#    # Restore original value.
#    self.dataset[sp.Data.Value, 'data2'] = self.reference_data2
#    np.testing.assert_array_equal(
#        self.dataset[sp.Data.Value, "data2"].numpy, self.reference_data2)
#
# def test_concatenate(self):
#    dataset = sp.Dataset()
#    dataset[sp.Data.Value, "data"] = ([sp.Dim.X], np.arange(4))
#    dataset[sp.Coord.X] = ([sp.Dim.X], np.array([3, 2, 4, 1]))
#    dataset = sp.concatenate(dataset, dataset, sp.Dim.X)
#    np.testing.assert_array_equal(
#        dataset[sp.Coord.X].numpy, np.array([3, 2, 4, 1, 3, 2, 4, 1]))
#    np.testing.assert_array_equal(
#        dataset[sp.Data.Value, "data"].numpy,
#        np.array([0, 1, 2, 3, 0, 1, 2, 3]))
#
# def test_concatenate_with_slice(self):
#    dataset = sp.Dataset()
#    dataset[sp.Data.Value, "data"] = ([sp.Dim.X], np.arange(4))
#    dataset[sp.Coord.X] = ([sp.Dim.X], np.array([3, 2, 4, 1]))
#    dataset = sp.concatenate(dataset, dataset[sp.Dim.X, 0:2], sp.Dim.X)
#    np.testing.assert_array_equal(
#        dataset[sp.Coord.X].numpy, np.array([3, 2, 4, 1, 3, 2]))
#    np.testing.assert_array_equal(
#        dataset[sp.Data.Value, "data"].numpy, np.array([0, 1, 2, 3, 0, 1]))
#
# def test_rebin(self):
#    dataset = sp.Dataset()
#    dataset[sp.Data.Value, "data"] = ([sp.Dim.X], np.array(10 * [1.0]))
#    dataset[sp.Data.Value, "data"].unit = sp.units.counts
#    dataset[sp.Coord.X] = ([sp.Dim.X], np.arange(11.0))
#    new_coord = sp.Variable([sp.Dim.X], np.arange(0.0, 11, 2))
#    dataset = sp.rebin(dataset, new_coord)
#    np.testing.assert_array_equal(
#        dataset[sp.Data.Value, "data"].numpy, np.array(5 * [2]))
#    np.testing.assert_array_equal(
#        dataset[sp.Coord.X].numpy, np.arange(0, 11, 2))
#
# def test_sort(self):
#    dataset = sp.Dataset()
#    dataset[sp.Data.Value, "data"] = ([sp.Dim.X], np.arange(4))
#    dataset[sp.Data.Value, "data2"] = ([sp.Dim.X], np.arange(4))
#    dataset[sp.Coord.X] = ([sp.Dim.X], np.array([3, 2, 4, 1]))
#    dataset = sp.sort(dataset, sp.Coord.X)
#    np.testing.assert_array_equal(
#        dataset[sp.Data.Value, "data"].numpy, np.array([3, 1, 0, 2]))
#    np.testing.assert_array_equal(
#        dataset[sp.Coord.X].numpy, np.array([1, 2, 3, 4]))
#    dataset = sp.sort(dataset, sp.Data.Value, "data")
#    np.testing.assert_array_equal(
#        dataset[sp.Data.Value, "data"].numpy, np.arange(4))
#    np.testing.assert_array_equal(
#        dataset[sp.Coord.X].numpy, np.array([3, 2, 4, 1]))
#
# def test_filter(self):
#    dataset = sp.Dataset()
#    dataset[sp.Data.Value, "data"] = ([sp.Dim.X], np.arange(4))
#    dataset[sp.Coord.X] = ([sp.Dim.X], np.array([3, 2, 4, 1]))
#    select = sp.Variable([sp.Dim.X], np.array([False, True, False, True]))
#    dataset = sp.filter(dataset, select)
#    np.testing.assert_array_equal(
#        dataset[sp.Data.Value, "data"].numpy, np.array([1, 3]))
#    np.testing.assert_array_equal(dataset[sp.Coord.X].numpy,
#                                  np.array([2, 1]))
#
#
# s TestDatasetExamples(unittest.TestCase):
# def test_table_example(self):
#    table = sp.Dataset()
#    table[sp.Coord.Row] = ([sp.Dim.Row], ['a', 'bb', 'ccc', 'dddd'])
#    self.assertSequenceEqual(table[sp.Coord.Row].data, [
#                             'a', 'bb', 'ccc', 'dddd'])
#    table[sp.Data.Value, "col1"] = ([sp.Dim.Row], [3.0, 2.0, 1.0, 0.0])
#    table[sp.Data.Value, "col2"] = ([sp.Dim.Row], np.arange(4.0))
#    self.assertEqual(len(table), 3)
#
#    table[sp.Data.Value, "sum"] = ([sp.Dim.Row],
#                                   (len(table[sp.Coord.Row]),))
#
#    for name, tag, col in table:
#        if not tag.is_coord and name != "sum":
#            table[sp.Data.Value, "sum"] += col
#    np.testing.assert_array_equal(
#        table[sp.Data.Value, "col2"].numpy, np.array([0, 1, 2, 3]))
#    np.testing.assert_array_equal(
#        table[sp.Data.Value, "sum"].numpy, np.array([3, 3, 3, 3]))
#
#    table = sp.concatenate(table, table, sp.Dim.Row)
#    np.testing.assert_array_equal(
#        table[sp.Data.Value, "col1"].numpy,
#        np.array([3, 2, 1, 0, 3, 2, 1, 0]))
#
#    table = sp.concatenate(table[sp.Dim.Row, 0:2], table[sp.Dim.Row, 5:7],
#                           sp.Dim.Row)
#    np.testing.assert_array_equal(
#        table[sp.Data.Value, "col1"].numpy, np.array([3, 2, 2, 1]))
#
#    table = sp.sort(table, sp.Data.Value, "col1")
#    np.testing.assert_array_equal(
#        table[sp.Data.Value, "col1"].numpy, np.array([1, 2, 2, 3]))
#
#    table = sp.sort(table, sp.Coord.Row)
#    np.testing.assert_array_equal(
#        table[sp.Data.Value, "col1"].numpy, np.array([3, 2, 2, 1]))
#
#    for i in range(1, len(table[sp.Coord.Row])):
#        table[sp.Dim.Row, i] += table[sp.Dim.Row, i - 1]
#
#    np.testing.assert_array_equal(
#        table[sp.Data.Value, "col1"].numpy, np.array([3, 5, 7, 8]))
#
#    table[sp.Data.Value, "exp1"] = (
#        [sp.Dim.Row], np.exp(table[sp.Data.Value, "col1"]))
#    table[sp.Data.Value, "exp1"] -= table[sp.Data.Value, "col1"]
#    np.testing.assert_array_equal(table[sp.Data.Value, "exp1"].numpy,
#                                  np.exp(np.array([3, 5, 7, 8]))
#                                  - np.array([3, 5, 7, 8]))
#
#    table += table
#    self.assertSequenceEqual(table[sp.Coord.Row].data, [
#                             'a', 'bb', 'bb', 'ccc'])
#
# def test_table_example_no_assert(self):
#    table = sp.Dataset()
#
#    # Add columns
#    table[sp.Coord.Row] = ([sp.Dim.Row], ['a', 'bb', 'ccc', 'dddd'])
#    table[sp.Data.Value, "col1"] = ([sp.Dim.Row], [3.0, 2.0, 1.0, 0.0])
#    table[sp.Data.Value, "col2"] = ([sp.Dim.Row], np.arange(4.0))
#    table[sp.Data.Value, "sum"] = ([sp.Dim.Row], (4,))
#
#    # Do something for each column (here: sum)
#    for name, tag, col in table:
#        if not tag.is_coord and name != "sum":
#            table[sp.Data.Value, "sum"] += col
#
#    # Append tables (append rows of second table to first)
#    table = sp.concatenate(table, table, sp.Dim.Row)
#
#    # Append tables sections (e.g., to remove rows from the middle)
#    table = sp.concatenate(table[sp.Dim.Row, 0:2], table[sp.Dim.Row, 5:7],
#                           sp.Dim.Row)
#
#    # Sort by column
#    table = sp.sort(table, sp.Data.Value, "col1")
#    # ... or another one
#    table = sp.sort(table, sp.Coord.Row)
#
#    # Do something for each row (here: cumulative sum)
#    for i in range(1, len(table[sp.Coord.Row])):
#        table[sp.Dim.Row, i] += table[sp.Dim.Row, i - 1]
#
#    # Apply numpy function to column, store result as a new column
#    table[sp.Data.Value, "exp1"] = (
#        [sp.Dim.Row], np.exp(table[sp.Data.Value, "col1"]))
#    # ... or as an existing column
#    table[sp.Data.Value, "exp1"] = np.sin(table[sp.Data.Value, "exp1"])
#
#    # Remove column
#    del table[sp.Data.Value, "exp1"]
#
#    # Arithmetics with tables (here: add two tables)
#    table += table
#
# def test_MDHistoWorkspace_example(self):
#    L = 30
#    d = sp.Dataset()
#
#    # Add bin-edge axis for X
#    d[sp.Coord.X] = ([sp.Dim.X], np.arange(L + 1).astype(np.float64))
#    # ... and normal axes for Y and Z
#    d[sp.Coord.Y] = ([sp.Dim.Y], np.arange(L))
#    d[sp.Coord.Z] = ([sp.Dim.Z], np.arange(L))
#
#    # Add data variables
#    d[sp.Data.Value, "temperature"] = (
#        [sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
#        np.random.normal(size=L * L * L).reshape([L, L, L]))
#    d[sp.Data.Value, "pressure"] = (
#        [sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
#        np.random.normal(size=L * L * L).reshape([L, L, L]))
#    # Add uncertainties, matching name implicitly links it to corresponding
#    # data
#    d[sp.Data.Variance, "temperature"] = (
#        [sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
#        d[sp.Data.Value, "temperature"].numpy)
#
#    # Uncertainties are propagated using grouping mechanism based on name
#    square = d * d
#    np.testing.assert_array_equal(
#        square[sp.Data.Variance, "temperature"].numpy,
#        2.0 * (d[sp.Data.Value, "temperature"].numpy**2
#               * d[sp.Data.Variance, "temperature"].numpy))
#
#    # Add the counts units
#    d[sp.Data.Value, "temperature"].unit = sp.units.counts
#    d[sp.Data.Value, "pressure"].unit = sp.units.counts
#    d[sp.Data.Variance, "temperature"].unit = sp.units.counts \
#        * sp.units.counts
#    # The square operation is now prevented because the resulting counts
#    # variance unit (counts^4) is not part of the supported units, i.e. the
#    # result of that operation makes little physical sense.
#    with self.assertRaisesRegex(RuntimeError,
#                                r"Unsupported unit as result of "
#                                r"multiplication: \(counts\^2\) \* "
#                                r"\(counts\^2\)"):
#        d = d * d
#
#    # Rebin the X axis
#    d = sp.rebin(d, sp.Variable(
#        [sp.Dim.X], np.arange(0, L + 1, 2).astype(np.float64)))
#    # Rebin to different axis for every y
#    # Our rebin implementatinon is broken for this case for now
#    # rebinned = sp.rebin(d, sp.Variable(sp.Coord.X, [sp.Dim.Y, sp.Dim.X],
#    #                  np.arange(0,2*L).reshape([L,2]).astype(np.float64)))
#
#    # Do something with numpy and insert result
#    d[sp.Data.Value, "dz(p)"] = ([sp.Dim.Z, sp.Dim.Y, sp.Dim.X],
#                                 np.gradient(d[sp.Data.Value, "pressure"],
#                                             d[sp.Coord.Z], axis=0))
#
#    # Truncate Y and Z axes
#    d = sp.Dataset(d[sp.Dim.Y, 10:20][sp.Dim.Z, 10:20])
#
#    # Mean along Y axis
#    meanY = sp.mean(d, sp.Dim.Y)
#    # Subtract from original, making use of automatic broadcasting
#    d -= meanY
#
#    # Extract a Z slice
#    sliceZ = sp.Dataset(d[sp.Dim.Z, 7])
#    self.assertEqual(len(sliceZ.dimensions), 2)
#
# def test_Workspace2D_example(self):
#    d = sp.Dataset()
#
#    d[sp.Coord.SpectrumNumber] = ([sp.Dim.Spectrum], np.arange(1, 101))
#
#    # Add a (common) time-of-flight axis
#    d[sp.Coord.Tof] = ([sp.Dim.Tof], np.arange(1000))
#
#    # Add data with uncertainties
#    d[sp.Data.Value, "sample1"] = (
#        [sp.Dim.Spectrum, sp.Dim.Tof],
#        np.random.exponential(size=100 * 1000).reshape([100, 1000]))
#    d[sp.Data.Variance, "sample1"] = d[sp.Data.Value, "sample1"]
#
#    # Create a mask and use it to extract some of the spectra
#    select = sp.Variable([sp.Dim.Spectrum], np.isin(
#        d[sp.Coord.SpectrumNumber], np.arange(10, 20)))
#    spectra = sp.filter(d, select)
#    self.assertEqual(spectra.dimensions.shape[1], 10)
#
#    # Direct representation of a simple instrument (more standard Mantid
#    # instrument representation is of course supported, this is just to
#    # demonstrate the flexibility)
#    steps = np.arange(-0.45, 0.46, 0.1)
#    x = np.tile(steps, (10,))
#    y = x.reshape([10, 10]).transpose().flatten()
#    d[sp.Coord.X] = ([sp.Dim.Spectrum], x)
#    d[sp.Coord.Y] = ([sp.Dim.Spectrum], y)
#    d[sp.Coord.Z] = ([], 10.0)
#
#    # Mask some spectra based on distance from beam center
#    r = np.sqrt(np.square(d[sp.Coord.X]) + np.square(d[sp.Coord.Y]))
#    d[sp.Coord.Mask] = ([sp.Dim.Spectrum], np.less(r, 0.2))
#
#    # Do something for each spectrum (here: apply mask)
#    d[sp.Coord.Mask].data
#    for i, masked in enumerate(d[sp.Coord.Mask].numpy):
#        spec = d[sp.Dim.Spectrum, i]
#        if masked:
#            spec[sp.Data.Value, "sample1"] = np.zeros(1000)
#            spec[sp.Data.Variance, "sample1"] = np.zeros(1000)
#
# def test_monitors_example(self):
#    d = sp.Dataset()
#
#    d[sp.Coord.SpectrumNumber] = ([sp.Dim.Spectrum], np.arange(1, 101))
#
#    # Add a (common) time-of-flight axis
#    d[sp.Coord.Tof] = ([sp.Dim.Tof], np.arange(9))
#
#    # Add data with uncertainties
#    d[sp.Data.Value, "sample1"] = (
#        [sp.Dim.Spectrum, sp.Dim.Tof],
#        np.random.exponential(size=100 * 8).reshape([100, 8]))
#    d[sp.Data.Variance, "sample1"] = d[sp.Data.Value, "sample1"]
#
#    # Add event-mode beam-status monitor
#    status = sp.Dataset()
#    status[sp.Data.Tof] = ([sp.Dim.Event],
#                           np.random.exponential(size=1000))
#    status[sp.Data.PulseTime] = (
#        [sp.Dim.Event], np.random.exponential(size=1000))
#    d[sp.Coord.Monitor, "beam-status"] = ([], status)
#
#    # Add position-resolved beam-profile monitor
#    profile = sp.Dataset()
#    profile[sp.Coord.X] = ([sp.Dim.X], [-0.02, -0.01, 0.0, 0.01, 0.02])
#    profile[sp.Coord.Y] = ([sp.Dim.Y], [-0.02, -0.01, 0.0, 0.01, 0.02])
#    profile[sp.Data.Value] = ([sp.Dim.Y, sp.Dim.X], (4, 4))
#    # Monitors can also be attributes, so they are not required to match in
#    # operations
#    d[sp.Attr.Monitor, "beam-profile"] = ([], profile)
#
#    # Add histogram-mode transmission monitor
#    transmission = sp.Dataset()
#    transmission[sp.Coord.Energy] = ([sp.Dim.Energy], np.arange(9))
#    transmission[sp.Data.Value] = (
#        [sp.Dim.Energy], np.random.exponential(size=8))
#    d[sp.Coord.Monitor, "transmission"] = ([], ())
#
# @unittest.skip("Tag-derived dtype not available anymore, need to implement \
#               way of specifying list type for events.")
# def test_zip(self):
#    d = sp.Dataset()
#    d[sp.Coord.SpectrumNumber] = ([sp.Dim.Position], np.arange(1, 6))
#    d[sp.Data.EventTofs, ""] = ([sp.Dim.Position], (5,))
#    d[sp.Data.EventPulseTimes, ""] = ([sp.Dim.Position], (5,))
#    self.assertEqual(len(d[sp.Data.EventTofs, ""].data), 5)
#    d[sp.Data.EventTofs, ""].data[0].append(10)
#    d[sp.Data.EventPulseTimes, ""].data[0].append(1000)
#    d[sp.Data.EventTofs, ""].data[1].append(10)
#    d[sp.Data.EventPulseTimes, ""].data[1].append(1000)
#    # Don't do this, there are no compatiblity checks:
#    # for el in zip(d[sp.Data.EventTofs, ""].data,
#    #     d[sp.Data.EventPulseTimes, ""].data):
#    for el, size in zip(d.zip(), [1, 1, 0, 0, 0]):
#        self.assertEqual(len(el), size)
#        for e in el:
#            self.assertEqual(e.first(), 10)
#            self.assertEqual(e.second(), 1000)
#        el.append((10, 300))
#        self.assertEqual(len(el), size + 1)
#
# def test_np_array_strides(self):
#    N = 6
#    M = 4
#    d1 = sp.Dataset()
#    d1[sp.Coord.X] = ([sp.Dim.X], np.arange(N + 1).astype(np.float64))
#    d1[sp.Coord.Y] = ([sp.Dim.Y], np.arange(M + 1).astype(np.float64))
#
#    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64)
#    arr2 = np.transpose(arr1)
#    K = 3
#    arr_buf = np.arange(N * K * M).reshape(N, K, M)
#    arr3 = arr_buf[:, 1, :]
#    d1[sp.Data.Value, "A"] = ([sp.Dim.X, sp.Dim.Y], arr1)
#    d1[sp.Data.Value, "B"] = ([sp.Dim.Y, sp.Dim.X], arr2)
#    d1[sp.Data.Value, "C"] = ([sp.Dim.X, sp.Dim.Y], arr3)
#    np.testing.assert_array_equal(arr1, d1[sp.Data.Value, "A"].numpy)
#    np.testing.assert_array_equal(arr2, d1[sp.Data.Value, "B"].numpy)
#    np.testing.assert_array_equal(arr3, d1[sp.Data.Value, "C"].numpy)
#
# def test_rebin(self):
#    N = 6
#    M = 4
#    d1 = sp.Dataset()
#    d1[sp.Coord.X] = ([sp.Dim.X], np.arange(N + 1).astype(np.float64))
#    d1[sp.Coord.Y] = ([sp.Dim.Y], np.arange(M + 1).astype(np.float64))
#
#    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64)
#    # TODO copy not needed after correct treatment of strides
#    arr2 = np.transpose(arr1).copy()
#
#    d1[sp.Data.Value, "A"] = ([sp.Dim.X, sp.Dim.Y], arr1)
#    d1[sp.Data.Value, "B"] = ([sp.Dim.Y, sp.Dim.X], arr2)
#    d1[sp.Data.Value, "A"].unit = sp.units.counts
#    d1[sp.Data.Value, "B"].unit = sp.units.counts
#    rd1 = sp.rebin(d1, sp.Variable([sp.Dim.X], np.arange(
#        0, N + 1, 1.5).astype(np.float64)))
#    np.testing.assert_array_equal(rd1[sp.Data.Value, "A"].numpy,
#                                  np.transpose(
#                                      rd1[sp.Data.Value, "B"].numpy))
#
# def test_copy(self):
#    import copy
#    N = 6
#    M = 4
#    d1 = sp.Dataset()
#    d1[sp.Coord.X] = ([sp.Dim.X], np.arange(N + 1).astype(np.float64))
#    d1[sp.Coord.Y] = ([sp.Dim.Y], np.arange(M + 1).astype(np.float64))
#    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
#    d1[sp.Data.Value, "A"] = ([sp.Dim.X, sp.Dim.Y], arr1)
#    d2 = copy.copy(d1)
#    d3 = copy.deepcopy(d2)
#    self.assertEqual(d1, d2)
#    self.assertEqual(d3, d2)
#    d2[sp.Data.Value, "A"] *= d2[sp.Data.Value, "A"]
#    self.assertNotEqual(d1, d2)
#    self.assertNotEqual(d3, d2)
#
# def test_correct_temporaries(self):
#    N = 6
#    M = 4
#    d1 = sp.Dataset()
#    d1[sp.Coord.X] = ([sp.Dim.X], np.arange(N + 1).astype(np.float64))
#    d1[sp.Coord.Y] = ([sp.Dim.Y], np.arange(M + 1).astype(np.float64))
#    arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
#    d1[sp.Data.Value, "A"] = ([sp.Dim.X, sp.Dim.Y], arr1)
#    d1 = d1[sp.Dim.X, 1:2]
#    self.assertEqual(list(d1[sp.Data.Value, "A"].data),
#                     [5.0, 6.0, 7.0, 8.0])
#    d1 = d1[sp.Dim.Y, 2:3]
#    self.assertEqual(list(d1[sp.Data.Value, "A"].data), [7])
