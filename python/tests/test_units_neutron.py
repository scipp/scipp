# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import pytest

import scipp as sc
from scipp import Dim


def test_dims():
    assert Dim.X == Dim.X
    assert Dim.X != Dim.Y
    assert Dim.Y != Dim.X
    assert Dim.Y == Dim.Y


def test_default_unit():
    u = sc.Unit()
    assert u == sc.units.dimensionless


def test_unit_repr():
    assert repr(sc.units.angstrom) == "\u212B"
    assert repr(sc.units.m) == "m"


def test_pow():
    assert sc.units.m**0 == sc.units.dimensionless
    assert sc.units.m**1 == sc.units.m
    assert sc.units.m**2 == sc.units.m * sc.units.m
    assert sc.units.m**3 == sc.units.m * sc.units.m * sc.units.m
    assert sc.units.m**-1 == sc.units.dimensionless / sc.units.m
    assert sc.units.m**-2 == sc.units.dimensionless / sc.units.m / sc.units.m
    with pytest.raises(RuntimeError):
        assert sc.units.m**27
