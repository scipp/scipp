import unittest

from dataset import *
import numpy as np

class TestUnits(unittest.TestCase):
    def test_create_unit(self):
        u = units.angstrom
        self.assertEqual(repr(u), "\u212B")

    def test_variable_unit(self):
        var1 = Variable(Data.Value, [Dim.X], np.arange(4))
        var2 = Variable(Coord.X, [Dim.X], np.arange(4))
        self.assertEqual(var1.unit, units.dimensionless)
        self.assertEqual(var2.unit, units.m)
        self.assertNotEqual(var2.unit, units.counts)
        self.assertTrue(var2.unit != units.counts)

    def test_variable_unit_repr(self):
        var1 = Variable(Data.Value, [Dim.X], np.arange(4))
        var2 = Variable(Coord.X, [Dim.X], np.arange(4))
        self.assertEqual(repr(var1.unit), "dimensionless")
        self.assertEqual(repr(var2.unit), "m")
        u = units.angstrom
        var1.unit = u
        self.assertEqual(repr(var1.unit), "\u212B")
        var1.unit = units.m * units.m
        self.assertEqual(repr(var1.unit), "m^2")
        var1.unit = units.counts / units.us
        self.assertEqual(repr(var1.unit), "counts \u03BCs^-1")
        with self.assertRaisesRegex(RuntimeError, "Unsupported unit as result of division: \(counts\) / \(m\)"):
            var1.unit = units.counts / units.m
        var1.unit = units.m / units.m * units.counts
        self.assertEqual(repr(var1.unit), "counts")
        # The statement below fails because (units.counts * units.m) is
        # performed first and is not part of the currently supported set of
        # intermediate units.
        with self.assertRaisesRegex(RuntimeError, "Unsupported unit as result of multiplication: \(counts\) \* \(m\)"):
            var1.unit = units.counts * units.m / units.m

if __name__ == '__main__':
    unittest.main()
