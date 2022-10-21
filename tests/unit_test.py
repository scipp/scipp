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
                               sc.Unit('12.3 * m/A*kg^2/rad^3'), sc.Unit('count')))
def test_dict_roundtrip(u):
    assert units_identical(sc.Unit.from_dict(u.to_dict()), u)


@pytest.fixture
def clean_unit_aliases():
    sc.units.clear_unit_aliases()
    yield
    sc.units.clear_unit_aliases()


def test_unit_alias_overrides_to_string(clean_unit_aliases):
    sc.units.add_unit_alias(name='clucks', unit='19.3 m*A')
    clucks = sc.Unit('19.3 m*A')
    assert str(clucks) == 'clucks'
    assert str(sc.Unit('one') / clucks) == '1/clucks'
    assert str(clucks**2) == 'clucks^2'
    assert str(clucks * sc.Unit('kg')) == 'clucks*kg'


def test_can_add_multiple_aliases(clean_unit_aliases):
    sc.units.add_unit_alias(name='clucks', unit='19.3 m*A')
    sc.units.add_unit_alias(name='dogyear', unit='4492800s')
    assert str(sc.Unit('4492800s')) == 'dogyear'
    assert str(sc.Unit('19.3 m*A')) == 'clucks'


def test_unit_alias_enables_conversion_from_string(clean_unit_aliases):
    sc.units.add_unit_alias(name='speed', unit='m/s')
    assert sc.Unit('speed') == sc.Unit('m/s')
    assert sc.Unit('1/speed') == sc.Unit('s/m')
    assert sc.Unit('speed/K') == sc.Unit('m/s/K')


def test_can_remove_unit_alias(clean_unit_aliases):
    sc.units.add_unit_alias(name='clucks', unit='19.3 m*A')
    sc.units.add_unit_alias(name='dogyear', unit='4492800s')

    sc.units.remove_unit_alias(name='dogyear')
    assert str(sc.Unit('19.3 m*A')) == 'clucks'
    assert 'dogyear' not in str(sc.Unit('4492800s'))
    with pytest.raises(sc.UnitError):
        sc.Unit('dogyear')

    sc.units.remove_unit_alias(name='clucks')
    assert 'clucks' not in str(sc.Unit('19.3 m*A'))
    assert 'dogyear' not in str(sc.Unit('4492800s'))
    with pytest.raises(sc.UnitError):
        sc.Unit('dogyear')
    with pytest.raises(sc.UnitError):
        sc.Unit('clucks')


def test_clear_unit_alias(clean_unit_aliases):
    sc.units.add_unit_alias(name='speed', unit='m/s')
    sc.units.add_unit_alias(name='chubby', unit='100kg')

    sc.units.clear_unit_aliases()
    assert 'speed' not in str(sc.Unit('m/s'))
    assert 'chubby' not in str(sc.Unit('100kg'))
    with pytest.raises(sc.UnitError):
        sc.Unit('speed')
    with pytest.raises(sc.UnitError):
        sc.Unit('chubby')


def test_removing_undefined_alias_does_nothing(clean_unit_aliases):
    sc.units.add_unit_alias(name='chubby', unit='100kg')

    sc.units.remove_unit_alias(name='clucks')
    assert str(sc.Unit('100kg')) == 'chubby'
    assert sc.Unit('chubby') == sc.Unit('100kg')


def test_unit_aliases_context_manager(clean_unit_aliases):
    with sc.units.unit_aliases(clucks='19.3 m*A', speed='m/s'):
        assert str(sc.Unit('19.3 m*A')) == 'clucks'
        assert str(sc.Unit('m/s')) == 'speed'
    assert 'clucks' not in str(sc.Unit('19.3 m*A'))
    assert 'speed' not in str(sc.Unit('m/s'))


def test_unit_aliases_context_manager_preserves_prior_alias(clean_unit_aliases):
    sc.units.add_unit_alias(name='dogyear', unit='4492800s')
    with sc.units.unit_aliases(clucks='19.3 m*A'):
        assert str(sc.Unit('19.3 m*A')) == 'clucks'
        assert str(sc.Unit('4492800s')) == 'dogyear'
    assert 'clucks' not in str(sc.Unit('19.3 m*A'))
    assert str(sc.Unit('4492800s')) == 'dogyear'
