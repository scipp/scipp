# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import unittest

import scipp as sc
import numpy as np
import operator


class TestDatasetSlice(unittest.TestCase):
    def setUp(self):
        var = sc.Variable(['x'], values=np.arange(10))
        d = sc.Dataset(data={'a': var, 'b': var}, coords={'x': var})
        self._d = d

    # Leave as is
    def test_type(self):
        ds_slice = self._d['a']
        self.assertEqual(type(ds_slice), sc.DataArrayView)

    def test_slice_with_range_datasetview_then_dataarrayview(self):
        sl = self._d['x', 1:-1]["a"].values
        ref = np.array([1, 2, 3, 4, 5, 6, 7, 8], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertTrue(np.allclose(sl, ref))
        # omitting range end
        sl = self._d['x', 1:]["a"].values
        ref = np.array([1, 2, 3, 4, 5, 6, 7, 8, 9], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertTrue(np.allclose(sl, ref))
        # omitting range begin
        sl = self._d['x', :-1]["a"].values
        ref = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertTrue(np.allclose(sl, ref))
        # omitting range both begin and end
        sl = self._d['x', :]["b"].values
        ref = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertEqual(np.allclose(sl, ref), True)

    def test_slice_with_range_dataarrayview_then_dataarrayview(self):
        sl = self._d['a']['x', 1:-1].values
        ref = np.array([1, 2, 3, 4, 5, 6, 7, 8], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertTrue(np.allclose(sl, ref))
        # omitting range end
        sl = self._d['a']['x', 1:].values
        ref = np.array([1, 2, 3, 4, 5, 6, 7, 8, 9], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertTrue(np.allclose(sl, ref))
        # omitting range begin
        sl = self._d['a']['x', :-1].values
        ref = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertTrue(np.allclose(sl, ref))
        # omitting range both begin and end
        sl = self._d['b']['x', :].values
        ref = np.array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9], dtype=np.int64)
        self.assertEqual(ref.shape, sl.shape)
        self.assertEqual(np.allclose(sl, ref), True)

    def test_slice_single_index_dataset_then_(self):
        self.assertEqual(self._d['x', -4]["a"].values, self._d['x',
                                                               6]["a"].values)
        self.assertEqual(self._d["a"]['x', -3].values, self._d["a"]['x',
                                                                    7].values)

    def test_slice_single_index(self):
        self.assertEqual(self._d['x', -4]["a"].values, self._d['x',
                                                               6]["a"].values)
        self.assertEqual(self._d["a"]['x', -3].values, self._d["a"]['x',
                                                                    7].values)

    def test_copy_datasetview(self):
        import copy
        N = 6
        M = 4
        d1 = sc.Dataset()
        d1.coords['x'] = sc.Variable(['x'],
                                     values=np.arange(N + 1).astype(
                                         np.float64))
        d1.coords['y'] = sc.Variable(['y'],
                                     values=np.arange(M + 1).astype(
                                         np.float64))
        arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
        d1['A'] = sc.Variable(['x', 'y'], values=arr1)
        s1 = d1['x', 2:]
        s2 = copy.copy(s1)
        s3 = copy.deepcopy(s2)
        self.assertEqual(s1, s2)
        self.assertEqual(s3, s2)
        s2 *= s2
        self.assertNotEqual(s1['A'], s2['A'])
        self.assertNotEqual(s3['A'], s2['A'])

    def test_copy_datasetview(self):
        import copy
        N = 6
        M = 4
        x = sc.Variable(['x'], values=np.arange(N + 1).astype(np.float64))
        y = sc.Variable(['y'], values=np.arange(M + 1).astype(np.float64))
        arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
        da = sc.DataArray(sc.Variable(['x', 'y'], values=arr1),
                          coords={
                              'x': x,
                              'y': y
                          })
        s1 = da['x', 2:]
        s2 = copy.copy(s1)
        s3 = copy.deepcopy(s2)
        self.assertEqual(s1, s2)
        self.assertEqual(s3, s2)
        s2 *= s2
        self.assertNotEqual(s1, s2)
        self.assertNotEqual(s3, s2)

    def _apply_test_op_rhs_ds_slice(self,
                                    op,
                                    a,
                                    b,
                                    data,
                                    lh_var_name="a",
                                    rh_var_name="b"):
        # Assume numpy operations are correct as comparitor
        with np.errstate(invalid='ignore'):
            op(data, b.data.values)
        op(a, b)
        # Desired nan comparisons
        np.testing.assert_equal(a.data.values, data)

    def test_binary_slice_rhs_operations(self):
        var = sc.Variable(['x'],
                          values=np.arange(10.0),
                          variances=np.arange(10.0))
        a_source = sc.DataArray(
            var, coords={'x': sc.Variable(['x'], values=np.arange(10))})
        b_source = sc.DataArray(
            var, coords={'x': sc.Variable(['x'], values=np.arange(10))})
        values = np.copy(var.values)
        variances = np.copy(var.variances)

        a = sc.DataArrayView(a_source)
        b = sc.DataArrayView(b_source)

        c = a + b
        # Variables "a" and "b" added despite different names
        self.assertTrue(np.array_equal(c.data.values, values + values))
        self.assertTrue(np.array_equal(c.data.variances,
                                       variances + variances))

        c = a - b
        # Variables "a" and "b" subtracted despite different names
        self.assertTrue(np.array_equal(c.data.values, values - values))
        self.assertTrue(np.array_equal(c.data.variances,
                                       variances + variances))

        c = a * b
        # Variables "a" and "b" multiplied despite different names
        self.assertTrue(np.array_equal(c.data.values, values * values))
        self.assertTrue(
            np.array_equal(c.data.variances,
                           variances * (values * values) * 2))

        c = a / b
        # Variables "a" and "b" divided despite different names
        with np.errstate(invalid='ignore'):
            np.testing.assert_equal(c.data.values, values / values)
            np.testing.assert_equal(c.data.variances,
                                    2 * variances / (values * values))

        self._apply_test_op_rhs_ds_slice(operator.iadd, a, b, values)
        self._apply_test_op_rhs_ds_slice(operator.isub, a, b, values)
        self._apply_test_op_rhs_ds_slice(operator.imul, a, b, values)
        self._apply_test_op_rhs_ds_slice(operator.itruediv, a, b, values)

    def _apply_test_op_rhs_variable(self,
                                    op,
                                    a,
                                    b,
                                    data,
                                    lh_var_name="a",
                                    rh_var_name="b"):
        # Assume numpy operations are correct as comparitor
        op(data, b.values)
        op(a, b)
        # Desired nan comparisons
        np.testing.assert_equal(a.data.values, data)

    def test_binary_variable_rhs_operations(self):
        data = np.ones(10, dtype='float64')
        var = sc.Variable(['x'], values=data)
        a = sc.DataArray(var)
        a_slice = sc.DataArrayView(a)

        c = a_slice + var
        self.assertTrue(np.array_equal(c.data.values, data + data))

        c = a_slice - var
        self.assertTrue(np.array_equal(c.data.values, data - data))

        c = a_slice * var
        self.assertTrue(np.array_equal(c.data.values, data * data))

        c = a_slice / var
        with np.errstate(invalid='ignore'):
            np.testing.assert_equal(c.data.values, data / data)

        self._apply_test_op_rhs_variable(operator.iadd, a, var, data)
        self._apply_test_op_rhs_variable(operator.isub, a, var, data)
        self._apply_test_op_rhs_variable(operator.imul, a, var, data)
        self._apply_test_op_rhs_variable(operator.itruediv, a, var, data)

    def test_binary_float_operations(self):
        da = sc.DataArray(
            data=sc.Variable(['x'], values=np.arange(10.0)),
            coords={'x': sc.Variable(['x'], values=np.arange(10))})
        a = sc.DataArrayView(da)
        b = sc.DataArrayView(da)
        data = np.copy(a.data.values)

        c = a + 2.0 * sc.units.dimensionless
        self.assertTrue(np.array_equal(c.data.values, data + 2.0))
        c = a - b
        self.assertTrue(np.array_equal(c.data.values, data - data))
        c = a - 2.0 * sc.units.dimensionless
        self.assertTrue(np.array_equal(c.data.values, data - 2.0))
        c = a * (2.0 * sc.units.dimensionless)
        self.assertTrue(np.array_equal(c.data.values, data * 2.0))
        c = a / (2.0 * sc.units.dimensionless)
        self.assertTrue(np.array_equal(c.data.values, data / 2.0))
        c = 2.0 * sc.units.dimensionless + a
        self.assertTrue(np.array_equal(c.data.values, data + 2.0))
        c = 2.0 * sc.units.dimensionless - a
        self.assertTrue(np.array_equal(c.data.values, 2.0 - data))
        c = 2.0 * sc.units.dimensionless * a
        self.assertTrue(np.array_equal(c.data.values, data * 2.0))

        def assert_same_result(op, x, y, data):
            op(data, y)
            op(x, y)
            np.testing.assert_equal(x.data.values, data)

        # Desired nan comparisons
        np.testing.assert_equal(a.data.values, data)
        assert_same_result(operator.iadd, a, 2.0, data)
        assert_same_result(operator.isub, a, 2.0, data)
        assert_same_result(operator.imul, a, 2.0, data)
        assert_same_result(operator.itruediv, a, 2.0, data)


'''
def test_slice_and_dimensions_items_dataarray():
    da = sc.DataArray(
        sc.Variable(['x', 'y'], values=np.arange(50).reshape(5, 10)))
    assert np.allclose(da['x', 0].values, da['x', 0:1].values[0], atol=1e-9)
    assert np.allclose(da['x', 4].values, da['x', -1].values)
    assert np.allclose(da['y', 1].values, da['y', -9].values)
    assert 'y' in da['x', 0].dims
    assert 'x' not in da['x', 0].dims
    assert 'y' in da['x', 0:1].dims
    assert 'x' in da['x', 0:1].dims


def test_slice_and_dimensions_items_dataset():
    da = sc.DataArray(
        sc.Variable(['x', 'y'], values=np.arange(50).reshape(5, 10)))
    ds = sc.Dataset({'a': da})
    assert np.allclose(ds['x', 0]['a'].values,
                       ds['x', 0:1]['a'].values[0],
                       atol=1e-9)
    assert np.allclose(ds['x', 4]['a'].values, ds['x', -1]['a'].values)
    assert np.allclose(ds['y', 1]['a'].values, ds['y', -9]['a'].values)
    assert 'y' in da['x', 0].dims
    assert 'x' not in da['x', 0].dims
    assert 'y' in da['x', 0:1].dims
    assert 'x' in da['x', 0:1].dims




    def test_equal_not_equal(self):
        d = sc.Dataset()
        d[sc.Coord.X] = (['x'], np.arange(10))
        d[sc.Data.Value, "a"] = (['x'], np.arange(10, dtype='float64'))
        d[sc.Data.Value, "b"] = (['x'], np.arange(10, dtype='float64'))
        a = d.subset["a"]
        b = d.subset["b"]
        c = a + b
        d2 = np.copy(a[sc.Data.Value, "a"].numpy)
        d2 = d['x', :]
        a2 = d.subset["a"]
        d3 = sc.Dataset()
        d3[sc.Coord.X] = (['x'], np.arange(10))
        d3[sc.Data.Value, "a"] = (['x'], np.arange(1, 11, dtype='float64'))
        a3 = d3.subset["a"]
        self.assertEqual(d, d2)
        self.assertEqual(d2, d)
        self.assertEqual(a, a2)
        self.assertNotEqual(a, b)
        self.assertNotEqual(b, a)
        self.assertNotEqual(a, c)
        self.assertNotEqual(a, a3)

    def test_binary_ops_with_variable(self):
        d = sc.Dataset()
        d[sc.Coord.X] = (['x'], np.arange(10))
        d[sc.Data.Value, "a"] = (['x'], np.arange(10))
        d[sc.Data.Variance, "a"] = (['x'], np.arange(10))

        d += sc.Variable(2)

    def test_correct_temporaries(self):
        N = 6
        M = 4
        d1 = sc.Dataset()
        d1[sc.Coord.X] = (['x'], np.arange(N + 1).astype(np.float64))
        d1[sc.Coord.Y] = (['y'], np.arange(M + 1).astype(np.float64))
        arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
        d1[sc.Data.Value, "A"] = (['x', 'y'], arr1)
        d1 = d1['x', 1:2]
        self.assertEqual(list(d1[sc.Data.Value, "A"].data),
                         [5.0, 6.0, 7.0, 8.0])

    def test_set_dataset_slice_items(self):
        d = self._d.copy()
        d[sc.Data.Value, "a"]['x', 0:2] += \
            d[sc.Data.Value, "b"]['x', 1:3]
        self.assertEqual(list(d[sc.Data.Value, "a"].data),
                         [1, 3, 2, 3, 4, 5, 6, 7, 8, 9])
        d[sc.Data.Value, "a"]['x', 6] += \
            d[sc.Data.Value, "b"]['x', 8]
        self.assertEqual(list(d[sc.Data.Value, "a"].data),
                         [1, 3, 2, 3, 4, 5, 14, 7, 8, 9])
'''

if __name__ == '__main__':
    unittest.main()
