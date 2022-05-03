# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest
import scipp as sc
import numpy as np


class TestUnits():

    def test_create_unit(self):
        u = sc.units.angstrom
        assert repr(u) == "\u212B" or repr(u) == "\u00C5"

    def test_variable_unit_repr(self):
        var1 = sc.Variable(dims=['x'], values=np.arange(4))
        u = sc.units.angstrom
        var1.unit = u
        assert repr(var1.unit) == "\u212B" or repr(var1.unit) == "\u00C5"
        var1.unit = sc.units.m * sc.units.m
        assert repr(var1.unit) == "m^2"
        var1.unit = sc.units.counts
        assert repr(var1.unit) == "counts"
        var1.unit = sc.units.counts / sc.units.m
        assert repr(var1.unit) == "counts/m"
        var1.unit = sc.units.m / sc.units.m * sc.units.counts
        assert repr(var1.unit) == "counts"
        var1.unit = sc.units.counts * sc.units.m / sc.units.m
        assert repr(var1.unit) == "counts"


def test_unit_property():
    var = sc.Variable(dims=(), values=1)
    var.unit = 'm'
    assert var.unit == sc.Unit('m')
    var.unit = sc.Unit('s')
    assert var.unit == sc.Unit('s')
    with pytest.raises(sc.UnitError):
        var.unit = 'abcdef'  # does not parse
    with pytest.raises(TypeError):
        var.unit = 5  # neither str nor Unit


def test_explicit_default_unit_for_string_gives_none():
    var = sc.scalar('abcdef', unit=sc.units.default_unit)
    assert var.unit is None


def test_default_unit_for_string_is_none():
    var = sc.scalar('abcdef')
    assert var.unit is None
