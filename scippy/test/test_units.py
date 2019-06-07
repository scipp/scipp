# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import unittest

import scippy as sp
import numpy as np


class TestUnits(unittest.TestCase):
    def test_create_unit(self):
        u = sp.units.angstrom
        self.assertEqual(repr(u), "\u212B")

    def test_variable_unit_repr(self):
        var1 = sp.Variable([sp.Dim.X], values=np.arange(4))
        u = sp.units.angstrom
        var1.unit = u
        self.assertEqual(repr(var1.unit), "\u212B")
        var1.unit = sp.units.m * sp.units.m
        self.assertEqual(repr(var1.unit), "m^2")
        var1.unit = sp.units.counts / sp.units.us
        self.assertEqual(repr(var1.unit), "counts \u03BCs^-1")
        with self.assertRaisesRegex(RuntimeError, r"Unsupported unit as "
                                    r"result of division: \(counts\) / \(m\)"):
            var1.unit = sp.units.counts / sp.units.m
        var1.unit = sp.units.m / sp.units.m * sp.units.counts
        self.assertEqual(repr(var1.unit), "counts")
        # The statement below fails because (sp.units.counts * sp.units.m) is
        # performed first and is not part of the currently supported set of
        # intermediate sp.units.
        with self.assertRaisesRegex(RuntimeError, r"Unsupported unit as "
                                    r"result of multiplication: "
                                    r"\(counts\) \* \(m\)"):
            var1.unit = sp.units.counts * sp.units.m / sp.units.m


if __name__ == '__main__':
    unittest.main()
