# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc


def test_default_unit():
    u = sc.Unit()
    assert u == sc.units.dimensionless


def test_unit_repr():
    assert (repr(sc.units.angstrom) == "\u212B") or (repr(sc.units.angstrom)
                                                     == "\u00C5")
    assert repr(sc.units.m) == "m"
    assert repr(sc.units.us) == "µs"
