# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import unittest

import scippy as sp
import numpy as np
import operator


class TestVariableSlice(unittest.TestCase):

    def setUp(self):
        self._a = sp.Variable(
            sp.Data.Value, [
                sp.Dim.X], np.arange(
                1, 10, dtype=float))
        self._b = sp.Variable(
            sp.Data.Value, [
                sp.Dim.X], np.arange(
                1, 10, dtype=float))

    def test_type(self):
        variable_slice = self._a[sp.Dim.X, :]
        self.assertEqual(type(variable_slice), sp.VariableSlice)

    def test_variable_type(self):
        coord_var = sp.Variable(sp.Coord.X, [sp.Dim.X], np.arange(10))
        coord_slice = coord_var[sp.Dim.X, :]
        self.assertTrue(coord_slice.is_coord)

    def _apply_test_op(self, op, a, b, data):
        op(a, b)
        # Assume numpy operations are correct as comparitor
        op(data, b.numpy)
        self.assertTrue(np.array_equal(a.numpy, data))

    def test_binary_operations(self):
        a = self._a[sp.Dim.X, :]
        b = self._b[sp.Dim.X, :]

        data = np.copy(a.numpy)
        c = a + b
        self.assertEqual(type(c), sp.Variable)
        self.assertTrue(np.array_equal(c.numpy, data + data))
        c = a - b
        self.assertTrue(np.array_equal(c.numpy, data - data))
        c = a * b
        self.assertTrue(np.array_equal(c.numpy, data * data))
        c = a / b
        self.assertTrue(np.array_equal(c.numpy, data / data))

        self._apply_test_op(operator.iadd, a, b, data)
        self._apply_test_op(operator.isub, a, b, data)
        self._apply_test_op(operator.imul, a, b, data)
        self._apply_test_op(operator.itruediv, a, b, data)

    def test_binary_float_operations(self):
        a = self._a[sp.Dim.X, :]
        data = np.copy(a.numpy)
        c = a + 2.0
        self.assertTrue(np.array_equal(c.numpy, data + 2.0))
        c = a - 2.0
        self.assertTrue(np.array_equal(c.numpy, data - 2.0))
        c = a * 2.0
        self.assertTrue(np.array_equal(c.numpy, data * 2.0))
        c = a / 2.0
        self.assertTrue(np.array_equal(c.numpy, data / 2.0))
        c = 2.0 + a
        self.assertTrue(np.array_equal(c.numpy, data + 2.0))
        c = 2.0 - a
        self.assertTrue(np.array_equal(c.numpy, 2.0 - data))
        c = 2.0 * a
        self.assertTrue(np.array_equal(c.numpy, data * 2.0))

    def test_equal_not_equal(self):
        a = self._a[sp.Dim.X, :]
        b = self._b[sp.Dim.X, :]
        c = a + 2.0
        self.assertEqual(a, b)
        self.assertEqual(b, a)
        self.assertNotEqual(a, c)
        self.assertNotEqual(c, a)

    def test_correct_temporaries(self):
        v = sp.Variable(
            sp.Data.Value, [
                sp.Dim.X], np.arange(100).astype(
                np.float32))
        b = sp.sqrt(v)[sp.Dim.X, 0:10]
        self.assertEqual(len(b.data), 10)
        b = b[sp.Dim.X, 2:5]
        self.assertEqual(len(b.data), 3)


if __name__ == '__main__':
    unittest.main()
