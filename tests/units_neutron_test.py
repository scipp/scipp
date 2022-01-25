# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
import pytest


def test_cannot_construct_unit_without_arguments():
    with pytest.raises(TypeError):
        sc.Unit()


def test_default_unit():
    u = sc.Unit('')
    assert u == sc.units.dimensionless


def test_unit_repr():
    assert (repr(sc.units.angstrom) == "\u212B") or (repr(
        sc.units.angstrom) == "\u00C5")
    assert repr(sc.units.m) == "m"
    assert repr(sc.units.us) == "µs"
