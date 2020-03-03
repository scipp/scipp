# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import unittest

import scipp as sc
import numpy as np


class TestUnits(unittest.TestCase):
    def test_create_unit(self):
        u = sc.units.angstrom
        self.assertEqual(repr(u), "\u212B")

    def test_variable_unit_repr(self):
        var1 = sc.Variable(['x'], values=np.arange(4))
        u = sc.units.angstrom
        var1.unit = u
        self.assertEqual(repr(var1.unit), "\u212B")
        var1.unit = sc.units.m * sc.units.m
        self.assertEqual(repr(var1.unit), "m^2")
        var1.unit = sc.units.counts / sc.units.us
        self.assertEqual(repr(var1.unit), "counts \u03BCs^-1")
        with self.assertRaisesRegex(
                RuntimeError, r"Unsupported unit as "
                r"result of division: \(counts\) / \(m\)"):
            var1.unit = sc.units.counts / sc.units.m
        var1.unit = sc.units.m / sc.units.m * sc.units.counts
        self.assertEqual(repr(var1.unit), "counts")
        # The statement below fails because (sc.units.counts * sc.units.m) is
        # performed first and is not part of the currently supported set of
        # intermediate sc.units.
        with self.assertRaisesRegex(
                RuntimeError, r"Unsupported unit as "
                r"result of multiplication: "
                r"\(counts\) \* \(m\)"):
            var1.unit = sc.units.counts * sc.units.m / sc.units.m


if __name__ == '__main__':
    unittest.main()
