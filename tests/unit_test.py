# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import copy
from collections.abc import Generator

import pytest

import scipp as sc
from scipp._scipp.core import units_identical as units_identical


@pytest.fixture(autouse=True)
def _clean_unit_aliases() -> Generator[None, None, None]:
    sc.units.aliases.clear()
    yield
    sc.units.aliases.clear()


def test_cannot_construct_unit_without_arguments() -> None:
    with pytest.raises(TypeError):
        sc.Unit()  # type: ignore[call-arg]


def test_default_unit() -> None:
    u = sc.Unit('')
    assert u == sc.units.dimensionless


def test_can_construct_unit_from_string() -> None:
    assert sc.Unit('m') == sc.units.m
    assert sc.Unit('meV') == sc.units.meV


def test_unit_from_unicode() -> None:
    assert sc.Unit('Å') == sc.Unit('angstrom')
    assert sc.Unit('µm') == sc.Unit('um')


def test_unit_equal_unicode() -> None:
    assert sc.Unit('angstrom') == 'Å'
    assert sc.Unit('us') == 'µs'


def test_constructor_raises_with_bad_input() -> None:
    with pytest.raises(sc.UnitError):
        sc.Unit('abcdef')  # does not parse
    with pytest.raises(TypeError):
        sc.Unit(5)  # type: ignore[arg-type] # neither str nor Unit


def test_unit_str_format() -> None:
    assert str(sc.units.m) == 'm'
    assert str(sc.units.us) == 'µs'


@pytest.mark.parametrize('u', [sc.units.angstrom, sc.Unit('angstrom')])
def test_angstrom_str_format(u: sc.Unit) -> None:
    assert str(u) in ('\u212b', '\u00c5')


def test_unit_repr() -> None:
    assert repr(sc.Unit('dimensionless')) == 'Unit(1)'
    assert repr(sc.Unit('m')) == 'Unit(m)'
    assert repr(sc.Unit('uK/rad')) == 'Unit(1e-06*K*rad**-1)'
    assert repr(sc.Unit('m^2/s^3')) == 'Unit(m**2*s**-3)'
    assert repr(sc.Unit('1.234*kg')) == 'Unit(1.234*kg)'
    assert repr(sc.Unit('degC')) == 'Unit(K, e_flag=True)'
    assert (
        repr(sc.Unit('decibels')) == 'Unit(1, i_flag=True, e_flag=True, equation=True)'
    )


def test_str() -> None:
    assert str(sc.Unit('kJ/mol')) == 'kJ/mol'


def test_degC_square() -> None:
    two_degC = sc.scalar(2.0, unit='degC')
    assert two_degC**2 == sc.scalar(4.0, unit='degC^2')
    assert sc.sqrt(two_degC**2) == two_degC


@pytest.mark.parametrize(
    'u', ['m', 'kg', 's', 'A', 'cd', 'K', 'mol', 'counts', '$', 'rad']
)
def test_unit_repr_uses_all_bases(u: str) -> None:
    assert repr(sc.Unit(u)) == f'Unit({u})'
    assert repr(sc.Unit(u) ** 2) == f'Unit({u}**2)'


def test_unit_property_from_str() -> None:
    var = sc.scalar(1)
    var.unit = 'm'
    assert var.unit == sc.Unit('m')


def test_unit_property_from_unit() -> None:
    var = sc.scalar(1)
    var.unit = sc.Unit('s')
    assert var.unit == sc.Unit('s')


def test_explicit_default_unit_for_string_gives_none() -> None:
    var = sc.scalar('abcdef', unit=sc.units.default_unit)
    assert var.unit is None


def test_default_unit_for_string_is_none() -> None:
    var = sc.scalar('abcdef')
    assert var.unit is None


@pytest.mark.parametrize(
    'u_str',
    [
        'one',
        'm',
        'count / s',
        '12.3 * m/A*kg^2/rad^3',
        'count',
        'decibels',
        'CXCUN[1]',
        'arbitraryunit',
        'EQXUN[1]',
        'Sv',
        'degC',
    ],
)
def test_dict_roundtrip(u_str: str) -> None:
    u = sc.Unit(u_str)
    assert units_identical(sc.Unit.from_dict(u.to_dict()), u)


@pytest.mark.parametrize('unit_type', [str, sc.Unit])
def test_unit_alias_overrides_str_formatting(unit_type: type) -> None:
    sc.units.aliases['clucks'] = unit_type('19.3 m*A')
    clucks = sc.Unit('19.3 m*A')
    assert str(clucks) == 'clucks'
    assert str(sc.Unit('one') / clucks) == '1/clucks'
    assert str(clucks**2) == 'clucks^2'
    assert str(clucks * sc.Unit('kg')) == 'clucks*kg'


def test_unit_alias_from_variable() -> None:
    sc.units.aliases['speed'] = sc.scalar(123, unit='mm/s')
    assert sc.Unit('speed') == sc.Unit('0.123 m/s')


def test_unit_alias_from_variable_cannot_have_variance() -> None:
    with pytest.raises(sc.VariancesError):
        sc.units.aliases['clucks'] = sc.scalar(19.3, variance=0.1, unit='m*A')


def test_unit_alias_from_variable_requires_scalar() -> None:
    with pytest.raises(sc.DimensionError):
        sc.units.aliases['nonsense'] = sc.array(dims=['x'], values=[1.0, 2.0], unit='m')


def test_unit_alias_does_not_affect_repr() -> None:
    sc.units.aliases['clucks'] = '19.3 m*A'
    assert repr(sc.Unit('19.3 m*A')) == 'Unit(19.3*m*A)'
    assert repr(sc.Unit('clucks')) == 'Unit(19.3*m*A)'


def test_can_add_multiple_aliases() -> None:
    sc.units.aliases['clucks'] = '19.3 m*A'
    sc.units.aliases['dogyear'] = '4492800s'
    assert str(sc.Unit('4492800s')) == 'dogyear'
    assert str(sc.Unit('19.3 m*A')) == 'clucks'


def test_can_add_aliases_for_different_scales() -> None:
    sc.units.aliases['long'] = '2*m'
    sc.units.aliases['short'] = '0.3*m'
    assert sc.Unit('long') == '2*m'
    assert sc.Unit('short') == '0.3*m'


def test_unit_alias_enables_conversion_from_string() -> None:
    sc.units.aliases['speed'] = 'm/s'
    assert sc.Unit('speed') == sc.Unit('m/s')
    assert sc.Unit('1/speed') == sc.Unit('s/m')
    assert sc.Unit('speed/K') == sc.Unit('m/s/K')


def test_can_override_alias() -> None:
    sc.units.aliases['speed'] = 'm/s'
    sc.units.aliases['speed'] = 'km/s'
    assert sc.Unit('speed') == 'km/s'


def test_can_redefine_same_alias() -> None:
    sc.units.aliases['speed'] = 'm/s'
    sc.units.aliases['speed'] = 'm/s'
    assert sc.Unit('speed') == 'm/s'
    sc.units.aliases['angstrom'] = 'angstrom'
    sc.units.aliases['angstrom'] = 'angstrom'
    assert sc.Unit('angstrom') == '10**-10 m'


def test_defining_conflicting_alias_raises() -> None:
    sc.units.aliases['speed'] = 'm/s'
    sc.units.aliases['zoomy'] = 'm/ms'
    with pytest.raises(ValueError, match='There already is an alias'):
        sc.units.aliases['fastness'] = 'm/s'
    with pytest.raises(ValueError, match='There already is an alias'):
        sc.units.aliases['fastness'] = '100*cm/s'
    with pytest.raises(ValueError, match='There already is an alias'):
        sc.units.aliases['fastness'] = sc.scalar(1000, unit='mm/s')
    with pytest.raises(ValueError, match='There already is an alias'):
        sc.units.aliases['zoomy'] = 'm/s'
    assert 'fastness' not in sc.units.aliases


def test_can_remove_unit_alias() -> None:
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


def test_clear_unit_alias() -> None:
    sc.units.aliases['speed'] = 'm/s'
    sc.units.aliases['chubby'] = '100kg'

    sc.units.aliases.clear()
    assert 'speed' not in str(sc.Unit('m/s'))
    assert 'chubby' not in str(sc.Unit('100kg'))
    with pytest.raises(sc.UnitError):
        sc.Unit('speed')
    with pytest.raises(sc.UnitError):
        sc.Unit('chubby')


def test_removing_undefined_alias_raises() -> None:
    with pytest.raises(KeyError):
        del sc.units.aliases['clucks']


def test_unit_aliases_context_manager() -> None:
    with sc.units.aliases.scoped(clucks='19.3 m*A', speed='m/s'):
        assert str(sc.Unit('19.3 m*A')) == 'clucks'
        assert str(sc.Unit('m/s')) == 'speed'
    assert 'clucks' not in str(sc.Unit('19.3 m*A'))
    assert 'speed' not in str(sc.Unit('m/s'))


def test_unit_aliases_context_manager_preserves_prior_alias() -> None:
    sc.units.aliases['dogyear'] = '4492800s'
    with sc.units.aliases.scoped(clucks='19.3 m*A'):
        assert str(sc.Unit('19.3 m*A')) == 'clucks'
        assert str(sc.Unit('4492800s')) == 'dogyear'
    assert 'clucks' not in str(sc.Unit('19.3 m*A'))
    assert str(sc.Unit('4492800s')) == 'dogyear'


def test_unit_aliases_context_manager_overrides_prior_alias() -> None:
    sc.units.aliases['speed'] = 'm/s'
    with sc.units.aliases.scoped(speed='km/s'):
        assert sc.Unit('speed') == 'km/s'
    assert sc.Unit('speed') == 'm/s'


def test_unit_aliases_context_manager_preserves_inner_alias() -> None:
    with sc.units.aliases.scoped(speed='m/s'):
        sc.units.aliases['dogyear'] = '4492800s'
        assert sc.Unit('speed') == 'm/s'
    assert str(sc.Unit('4492800s')) == 'dogyear'


def test_unit_aliases_context_manager_always_restores_prior_alias() -> None:
    sc.units.aliases['speed'] = 'm/s'
    with sc.units.aliases.scoped(speed='km/s'):
        sc.units.aliases['speed'] = 'um/s'
    assert sc.Unit('speed') == 'm/s'  # inner alias removed


def test_unit_aliases_context_manager_without_args() -> None:
    sc.units.aliases['clucks'] = '19.3 m*A'
    with sc.units.aliases.scoped():
        assert str(sc.Unit('19.3 m*A')) == 'clucks'
    assert str(sc.Unit('19.3 m*A')) == 'clucks'


def test_unit_aliases_iterate_over_aliases() -> None:
    sc.units.aliases['clucks'] = '19.3 m*A'
    sc.units.aliases['dogyear'] = '4492800s'
    assert sorted(sc.units.aliases) == ['clucks', 'dogyear']
    assert sorted(sc.units.aliases.keys()) == ['clucks', 'dogyear']
    assert sorted(sc.units.aliases.values(), key=lambda u: repr(u)) == [
        sc.Unit('19.3 m*A'),
        sc.Unit('4492800s'),
    ]
    assert sorted(sc.units.aliases.items()) == [
        ('clucks', sc.Unit('19.3 m*A')),
        ('dogyear', sc.Unit('4492800s')),
    ]


def test_aliases_is_a_singleton() -> None:
    with pytest.raises(RuntimeError):
        _ = sc.units.UnitAliases()
    with pytest.raises(TypeError):
        copy.copy(sc.units.aliases)
    with pytest.raises(TypeError):
        copy.deepcopy(sc.units.aliases)
