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
        d = sc.Dataset(data={'a':var, 'b':var}, coords={'x': var})
        self._d = d

    def test_type(self):
        ds_slice = self._d['a']
        self.assertEqual(type(ds_slice), sc.DataArrayView)


    def test_slice_with_range(self):
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
'''

    def test_slice_single_index(self):
        self.assertEqual(self._d['x', -4][sc.Data.Value, "a"].numpy,
                         self._d['x', 6][sc.Data.Value, "a"].numpy)
        self.assertEqual(self._d[sc.Data.Value, "a"]['x', -3].numpy,
                         self._d[sc.Data.Value, "a"]['x', 7].numpy)

    def test_range_based_slice(self):
        subset = slice(1, 4, 1)
        # Create slice
        ds_slice = self._d['x', subset]
        # Test via variable_slice
        self.assertEqual(len(ds_slice[sc.Coord.X]),
                         len(range(subset.start, subset.stop, subset.step)))

    def test_copy(self):
        import copy
        N = 6
        M = 4
        d1 = sc.Dataset()
        d1[sc.Coord.X] = (['x'], np.arange(N + 1).astype(np.float64))
        d1[sc.Coord.Y] = (['y'], np.arange(M + 1).astype(np.float64))
        arr1 = np.arange(N * M).reshape(N, M).astype(np.float64) + 1
        d1[sc.Data.Value, "A"] = (['x', 'y'], arr1)
        s1 = d1['x', 2:]
        s2 = copy.copy(s1)
        s3 = copy.deepcopy(s2)
        self.assertEqual(s1, s2)
        self.assertEqual(s3, s2)
        s2 *= s2
        self.assertNotEqual(s1[sc.Data.Value, "A"], s2[sc.Data.Value, "A"])
        self.assertNotEqual(s3[sc.Data.Value, "A"], s2[sc.Data.Value, "A"])

    def _apply_test_op_rhs_ds_slice(self,
                                    op,
                                    a,
                                    b,
                                    data,
                                    lh_var_name="a",
                                    rh_var_name="b"):
        # Assume numpy operations are correct as comparitor
        with np.errstate(invalid='ignore'):
            op(data, b[sc.Data.Value, rh_var_name].numpy)
        op(a, b)
        # Desired nan comparisons
        np.testing.assert_equal(a[sc.Data.Value, lh_var_name].numpy, data)

    def _apply_test_op_rhs_variable(self,
                                    op,
                                    a,
                                    b,
                                    data,
                                    lh_var_name="a",
                                    rh_var_name="b"):
        # Assume numpy operations are correct as comparitor
        op(data, b.numpy)
        op(a, b)
        # Desired nan comparisons
        np.testing.assert_equal(a[sc.Data.Value, lh_var_name].numpy, data)

    def test_binary_slice_rhs_operations(self):
        d = sc.Dataset()
        d[sc.Coord.X] = (['x'], np.arange(10))
        d[sc.Data.Value, "a"] = (['x'], np.arange(10, dtype='float64'))
        d[sc.Data.Variance, "a"] = (['x'], np.arange(10, dtype='float64'))
        d[sc.Data.Value, "b"] = (['x'], np.arange(10, dtype='float64'))
        d[sc.Data.Variance, "b"] = (['x'], np.arange(10, dtype='float64'))
        a = d.subset["a"]
        b = d.subset["b"]
        data = np.copy(a[sc.Data.Value, "a"].numpy)
        variance = np.copy(a[sc.Data.Variance, "a"].numpy)

        c = a + b
        # Variables "a" and "b" added despite different names
        self.assertTrue(
            np.array_equal(c[sc.Data.Value, "a"].numpy, data + data))
        self.assertTrue(
            np.array_equal(c[sc.Data.Variance, "a"].numpy,
                           variance + variance))

        c = a - b
        # Variables "a" and "b" subtracted despite different names
        self.assertTrue(
            np.array_equal(c[sc.Data.Value, "a"].numpy, data - data))
        self.assertTrue(
            np.array_equal(c[sc.Data.Variance, "a"].numpy,
                           variance + variance))

        c = a * b
        # Variables "a" and "b" multiplied despite different names
        self.assertTrue(
            np.array_equal(c[sc.Data.Value, "a"].numpy, data * data))
        self.assertTrue(
            np.array_equal(c[sc.Data.Variance, "a"].numpy,
                           variance * (data * data) * 2))

        c = a / b
        # Variables "a" and "b" divided despite different names
        with np.errstate(invalid='ignore'):
            np.testing.assert_equal(c[sc.Data.Value, "a"].numpy, data / data)
            np.testing.assert_equal(c[sc.Data.Variance, "a"].numpy,
                                    2 * variance / (data * data))

        self._apply_test_op_rhs_ds_slice(operator.iadd, a, b, data)
        self._apply_test_op_rhs_ds_slice(operator.isub, a, b, data)
        self._apply_test_op_rhs_ds_slice(operator.imul, a, b, data)
        self._apply_test_op_rhs_ds_slice(operator.itruediv, a, b, data)

    def test_binary_variable_rhs_operations(self):
        data = np.ones(10, dtype='float64')
        d = sc.Dataset()
        d[sc.Data.Value, "a"] = (['x'], data)
        d[sc.Data.Variance, "a"] = (['x'], data)
        a = d.subset[sc.Data.Value, "a"]
        b_var = sc.Variable(['x'], data)

        c = a + b_var
        self.assertTrue(
            np.array_equal(c[sc.Data.Value, "a"].numpy, data + data))

        c = a - b_var
        self.assertTrue(
            np.array_equal(c[sc.Data.Value, "a"].numpy, data - data))

        c = a * b_var
        self.assertTrue(
            np.array_equal(c[sc.Data.Value, "a"].numpy, data * data))

        c = a / b_var
        with np.errstate(invalid='ignore'):
            np.testing.assert_equal(c[sc.Data.Value, "a"].numpy, data / data)

        self._apply_test_op_rhs_variable(operator.iadd, a, b_var, data)
        self._apply_test_op_rhs_variable(operator.isub, a, b_var, data)
        self._apply_test_op_rhs_variable(operator.imul, a, b_var, data)
        self._apply_test_op_rhs_variable(operator.itruediv, a, b_var, data)

    def test_binary_float_operations(self):
        d = sc.Dataset()
        d[sc.Coord.X] = (['x'], np.arange(10))
        d[sc.Data.Value, "a"] = (['x'], np.arange(10, dtype='float64'))
        d[sc.Data.Value, "b"] = (['x'], np.arange(10, dtype='float64'))
        a = d.subset["a"]
        b = d.subset["b"]
        data = np.copy(a[sc.Data.Value, "a"].numpy)

        c = a + 2.0
        self.assertTrue(np.array_equal(c[sc.Data.Value, "a"].numpy,
                                       data + 2.0))
        c = a - b
        self.assertTrue(
            np.array_equal(c[sc.Data.Value, "a"].numpy, data - data))
        c = a - 2.0
        self.assertTrue(np.array_equal(c[sc.Data.Value, "a"].numpy,
                                       data - 2.0))
        c = a * 2.0
        self.assertTrue(np.array_equal(c[sc.Data.Value, "a"].numpy,
                                       data * 2.0))
        c = a / 2.0
        self.assertTrue(np.array_equal(c[sc.Data.Value, "a"].numpy,
                                       data / 2.0))
        c = 2.0 + a
        self.assertTrue(np.array_equal(c[sc.Data.Value, "a"].numpy,
                                       data + 2.0))
        c = 2.0 - a
        self.assertTrue(np.array_equal(c[sc.Data.Value, "a"].numpy,
                                       2.0 - data))
        c = 2.0 * a
        self.assertTrue(np.array_equal(c[sc.Data.Value, "a"].numpy,
                                       data * 2.0))

        self._apply_test_op_rhs_ds_slice(operator.iadd, a, b, data)
        self._apply_test_op_rhs_ds_slice(operator.isub, a, b, data)
        self._apply_test_op_rhs_ds_slice(operator.imul, a, b, data)
        self._apply_test_op_rhs_ds_slice(operator.itruediv, a, b, data)

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
