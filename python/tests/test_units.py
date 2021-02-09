# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
import numpy as np


class TestUnits():
    def test_create_unit(self):
        u = sc.units.angstrom
        assert repr(u) == "\u212B" or repr(u) == "\u00C5"

    def test_variable_unit_repr(self):
        var1 = sc.Variable(['x'], values=np.arange(4))
        u = sc.units.angstrom
        var1.unit = u
        assert repr(var1.unit) == "\u212B" or repr(var1.unit) == "\u00C5"
        var1.unit = sc.units.m * sc.units.m
        assert repr(var1.unit) == "m^2"
        var1.unit = sc.units.counts / sc.units.m
        assert repr(var1.unit) == "counts/m"
        var1.unit = sc.units.m / sc.units.m * sc.units.counts
        assert repr(var1.unit) == "counts"
        var1.unit = sc.units.counts * sc.units.m / sc.units.m
        assert repr(var1.unit) == "counts"
