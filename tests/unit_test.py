# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
from scipp._scipp.core import units_identical as units_identical  # noqa
import scipp as sc
import pytest


def test_cannot_construct_unit_without_arguments():
    with pytest.raises(TypeError):
        sc.Unit()  # noqa


def test_default_unit():
    u = sc.Unit('')
    assert u == sc.units.dimensionless


def test_can_construct_unit_from_string():
    assert sc.Unit('m') == sc.units.m
    assert sc.Unit('meV') == sc.units.meV


def test_unit_from_unicode():
    assert sc.Unit('Å') == sc.Unit('angstrom')
    assert sc.Unit('µm') == sc.Unit('um')


def test_unit_equal_unicode():
    assert sc.Unit('angstrom') == 'Å'
    assert sc.Unit('us') == 'µs'


def test_constructor_raises_with_bad_input():
    with pytest.raises(sc.UnitError):
        sc.Unit('abcdef')  # does not parse
    with pytest.raises(TypeError):
        sc.Unit(5)  # neither str nor Unit


def test_unit_repr():
    assert repr(sc.units.m) == 'm'
    assert repr(sc.units.us) == 'µs'


@pytest.mark.parametrize('u', (sc.units.angstrom, sc.Unit('angstrom')))
def test_angstrom_repr(u):
    assert repr(u) == '\u212B' or repr(u) == '\u00C5'


def test_unit_property_from_str():
    var = sc.scalar(1)
    var.unit = 'm'
    assert var.unit == sc.Unit('m')


def test_unit_property_from_unit():
    var = sc.scalar(1)
    var.unit = sc.Unit('s')
    assert var.unit == sc.Unit('s')


def test_explicit_default_unit_for_string_gives_none():
    var = sc.scalar('abcdef', unit=sc.units.default_unit)
    assert var.unit is None


def test_default_unit_for_string_is_none():
    var = sc.scalar('abcdef')
    assert var.unit is None


@pytest.mark.parametrize('u', (sc.Unit('one'), sc.Unit('m'), sc.Unit('count / s'),
                               sc.Unit('12.3 * m/A*kg^2/rad^3'), sc.Unit('CXUN[573]')))
def test_dict_roundtrip(u):
    assert units_identical(sc.Unit.from_dict(u.to_dict()), u)
