# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sp
from scipp import Dim


def test_dims():
    assert Dim.X == Dim.X
    assert Dim.X != Dim.Y
    assert Dim.Y != Dim.X
    assert Dim.Y == Dim.Y


def test_default_unit():
    u = sp.Unit()
    assert u == sp.units.dimensionless


def test_unit_repr():
    assert repr(sp.units.angstrom) == "\u212B"
    assert repr(sp.units.m) == "m"
