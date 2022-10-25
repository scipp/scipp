# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import copy

from scipp._scipp.core import units_identical as units_identical  # noqa
import scipp as sc
import pytest


@pytest.fixture(autouse=True)
def clean_unit_aliases():
    sc.units.aliases.clear()
    yield
    sc.units.aliases.clear()


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
        sc.Unit(5)  # type: ignore # neither str nor Unit


def test_unit_str_format():
    assert str(sc.units.m) == 'm'
    assert str(sc.units.us) == 'µs'


@pytest.mark.parametrize('u', (sc.units.angstrom, sc.Unit('angstrom')))
def test_angstrom_str_format(u):
    assert str(u) in ('\u212B', '\u00C5')


def test_unit_repr():
    assert repr(sc.Unit('dimensionless')) == 'Unit(1)'
    assert repr(sc.Unit('m')) == 'Unit(1*m**1)'
    assert repr(sc.Unit('uK/rad')) == 'Unit(1e-06*K**1*rad**-1)'
    assert repr(sc.Unit('m^2/s^3')) == 'Unit(1*m**2*s**-3)'
    assert repr(sc.Unit('1.234*kg')) == 'Unit(1.234*kg**1)'


@pytest.mark.parametrize('u',
                         ('m', 'kg', 's', 'A', 'cd', 'K', 'mol', 'count', '$', 'rad'))
def test_unit_repr_uses_all_bases(u):
    assert repr(sc.Unit(u)) == f'Unit(1*{u}**1)'


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


@pytest.mark.parametrize('unit_type', (str, sc.Unit))
def test_unit_alias_overrides_str_formatting(unit_type):
    sc.units.aliases['clucks'] = unit_type('19.3 m*A')
    clucks = sc.Unit('19.3 m*A')
    assert str(clucks) == 'clucks'
    assert str(sc.Unit('one') / clucks) == '1/clucks'
    assert str(clucks**2) == 'clucks^2'
    assert str(clucks * sc.Unit('kg')) == 'clucks*kg'


def test_unit_alias_does_not_affect_repr():
    sc.units.aliases['clucks'] = '19.3 m*A'
    assert repr(sc.Unit('19.3 m*A')) == 'Unit(19.3*m**1*A**1)'
    assert repr(sc.Unit('clucks')) == 'Unit(19.3*m**1*A**1)'


def test_can_add_multiple_aliases():
    sc.units.aliases['clucks'] = '19.3 m*A'
    sc.units.aliases['dogyear'] = '4492800s'
    assert str(sc.Unit('4492800s')) == 'dogyear'
    assert str(sc.Unit('19.3 m*A')) == 'clucks'


def test_unit_alias_enables_conversion_from_string():
    sc.units.aliases['speed'] = 'm/s'
    assert sc.Unit('speed') == sc.Unit('m/s')
    assert sc.Unit('1/speed') == sc.Unit('s/m')
    assert sc.Unit('speed/K') == sc.Unit('m/s/K')


def test_can_override_alias():
    sc.units.aliases['speed'] = 'm/s'
    sc.units.aliases['speed'] = 'km/s'
    assert sc.Unit('speed') == 'km/s'


def test_defining_conflicting_alias_raises():
    sc.units.aliases['speed'] = 'm/s'
    with pytest.raises(ValueError):
        sc.units.aliases['fastness'] = 'm/s'


def test_can_remove_unit_alias():
    sc.units.aliases['clucks'] = '19.3 m*A'
    sc.units.aliases['dogyear'] = '4492800s'

    del sc.units.aliases['dogyear']
    assert str(sc.Unit('19.3 m*A')) == 'clucks'
    assert 'dogyear' not in str(sc.Unit('4492800s'))
    with pytest.raises(sc.UnitError):
        sc.Unit('dogyear')

    del sc.units.aliases['clucks']
    assert 'clucks' not in str(sc.Unit('19.3 m*A'))
    assert 'dogyear' not in str(sc.Unit('4492800s'))
    with pytest.raises(sc.UnitError):
        sc.Unit('dogyear')
    with pytest.raises(sc.UnitError):
        sc.Unit('clucks')


def test_clear_unit_alias():
    sc.units.aliases['speed'] = 'm/s'
    sc.units.aliases['chubby'] = '100kg'

    sc.units.aliases.clear()
    assert 'speed' not in str(sc.Unit('m/s'))
    assert 'chubby' not in str(sc.Unit('100kg'))
    with pytest.raises(sc.UnitError):
        sc.Unit('speed')
    with pytest.raises(sc.UnitError):
        sc.Unit('chubby')


def test_removing_undefined_alias_raises():
    with pytest.raises(KeyError):
        del sc.units.aliases['clucks']


def test_unit_aliases_context_manager():
    with sc.units.aliases.scoped(clucks='19.3 m*A', speed='m/s'):
        assert str(sc.Unit('19.3 m*A')) == 'clucks'
        assert str(sc.Unit('m/s')) == 'speed'
    assert 'clucks' not in str(sc.Unit('19.3 m*A'))
    assert 'speed' not in str(sc.Unit('m/s'))


def test_unit_aliases_context_manager_preserves_prior_alias():
    sc.units.aliases['dogyear'] = '4492800s'
    with sc.units.aliases.scoped(clucks='19.3 m*A'):
        assert str(sc.Unit('19.3 m*A')) == 'clucks'
        assert str(sc.Unit('4492800s')) == 'dogyear'
    assert 'clucks' not in str(sc.Unit('19.3 m*A'))
    assert str(sc.Unit('4492800s')) == 'dogyear'


def test_unit_aliases_context_manager_overrides_prior_alias():
    sc.units.aliases['speed'] = 'm/s'
    with sc.units.aliases.scoped(speed='km/s'):
        assert sc.Unit('speed') == 'km/s'
    assert sc.Unit('speed') == 'm/s'


def test_unit_aliases_context_manager_preserves_inner_alias():
    with sc.units.aliases.scoped(speed='m/s'):
        sc.units.aliases['dogyear'] = '4492800s'
        assert sc.Unit('speed') == 'm/s'
    assert str(sc.Unit('4492800s')) == 'dogyear'


def test_unit_aliases_context_manager_always_restores_prior_alias():
    sc.units.aliases['speed'] = 'm/s'
    with sc.units.aliases.scoped(speed='km/s'):
        sc.units.aliases['speed'] = 'um/s'
    assert sc.Unit('speed') == 'm/s'  # inner alias removed


def test_unit_aliases_context_manager_without_args():
    sc.units.aliases['clucks'] = '19.3 m*A'
    with sc.units.aliases.scoped():
        assert str(sc.Unit('19.3 m*A')) == 'clucks'
    assert str(sc.Unit('19.3 m*A')) == 'clucks'


def test_unit_aliases_iterate_over_aliases():
    sc.units.aliases['clucks'] = '19.3 m*A'
    sc.units.aliases['dogyear'] = '4492800s'
    assert sorted(sc.units.aliases) == ['clucks', 'dogyear']
    assert sorted(sc.units.aliases.keys()) == ['clucks', 'dogyear']
    assert sorted(sc.units.aliases.values(),
                  key=lambda u: repr(u)) == [sc.Unit('19.3 m*A'),
                                             sc.Unit('4492800s')]
    assert sorted(sc.units.aliases.items()) == [('clucks', sc.Unit('19.3 m*A')),
                                                ('dogyear', sc.Unit('4492800s'))]


def test_aliases_is_a_singleton():
    with pytest.raises(RuntimeError):
        _ = sc.units.UnitAliases()
    with pytest.raises(TypeError):
        copy.copy(sc.units.aliases)
    with pytest.raises(TypeError):
        copy.deepcopy(sc.units.aliases)
