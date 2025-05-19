# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import numpy as np
import pytest

import scipp as sc


def test_comparison_operators() -> None:
    assert not sc.scalar(5) < sc.scalar(3)
    assert not sc.scalar(5) <= sc.scalar(3)
    assert sc.scalar(5) > sc.scalar(3)
    assert sc.scalar(5) >= sc.scalar(3)
    assert not sc.scalar(5) == sc.scalar(3)
    assert sc.scalar(5) != sc.scalar(3)


def test_comparison_functions() -> None:
    assert sc.less(sc.scalar(1), sc.scalar(2))
    assert sc.less_equal(sc.scalar(1), sc.scalar(2))
    assert not sc.greater(sc.scalar(1), sc.scalar(2))
    assert not sc.greater_equal(sc.scalar(1), sc.scalar(2))
    assert not sc.equal(sc.scalar(1), sc.scalar(2))
    assert sc.not_equal(sc.scalar(1), sc.scalar(2))


def test_isclose() -> None:
    unit = sc.units.m
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.identical(
        sc.isclose(a, a, rtol=0 * sc.units.one, atol=0 * unit),
        sc.full(sizes={'x': 3}, value=True),
    )


def test_isclose_atol_defaults() -> None:
    unit = sc.units.s
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.identical(
        sc.isclose(a, a, rtol=0 * sc.units.one), sc.full(sizes={'x': 3}, value=True)
    )


def test_isclose_rtol_defaults() -> None:
    unit = sc.units.kg
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.identical(
        sc.isclose(a, a, atol=0 * unit), sc.full(sizes={'x': 3}, value=True)
    )


def test_isclose_no_unit() -> None:
    a = sc.array(dims=['x'], values=[1, 2, 3], unit=None)
    assert sc.identical(sc.isclose(a, a), sc.full(sizes={'x': 3}, value=True))


def test_allclose() -> None:
    unit = sc.units.mm
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.allclose(a, a, rtol=0 * sc.units.one, atol=0 * unit)
    b = a.copy()
    b['x', 0] = 2
    assert not sc.allclose(a, b, rtol=0 * sc.units.one, atol=0 * unit)


def test_allclose_vectors() -> None:
    unit = sc.units.m
    a = sc.vectors(dims=['x'], values=[[1, 2, 3]], unit=unit)
    assert sc.allclose(a, a, atol=0 * unit)


def test_allclose_atol_defaults() -> None:
    unit = sc.units.m
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.allclose(a, a, rtol=0 * sc.units.one)


def test_allclose_rtol_defaults() -> None:
    unit = sc.units.s
    a = sc.Variable(dims=['x'], values=np.array([1, 2, 3]), unit=unit)
    assert sc.allclose(a, a, atol=0 * unit)


def test_allclose_no_unit() -> None:
    a = sc.array(dims=['x'], values=[1, 2, 3], unit=None)
    assert sc.allclose(a, a)


@pytest.mark.parametrize(
    't', [lambda x: x, sc.DataArray, lambda x: sc.Dataset({'a': x})]
)
def test_identical(t) -> None:
    assert sc.identical(t(sc.scalar(1.23)), t(sc.scalar(1.23)))
    assert not sc.identical(t(sc.scalar(1.23)), t(sc.scalar(1.23, unit='m')))
    assert not sc.identical(t(sc.scalar(1.23)), t(sc.scalar(5.2)))
    assert not sc.identical(t(sc.scalar(1.23)), t(sc.array(dims=['x'], values=[1.23])))
    assert sc.identical(
        t(sc.array(dims=['x'], values=[1.23], unit='m')),
        t(sc.array(dims=['x'], values=[1.23], unit='m')),
    )


def test_identical_data_group() -> None:
    dg1 = sc.DataGroup(
        {
            'variable': sc.scalar(4.182, unit='s'),
            'data array': sc.DataArray(sc.scalar(52), coords={'x': sc.scalar('abc')}),
            'data group': sc.DataGroup(
                {
                    '1': sc.arange('x', 5),
                }
            ),
            'tråde': 'Ét eksempel',
            'a number': 123,
            'dict': {'k': 'value1', 'n': 77},
            'numpy-array': np.array([1, 6, 23]),
        }
    )
    assert sc.identical(dg1, dg1.copy())


def test_identical_data_group_disordered() -> None:
    dg1 = sc.DataGroup(
        {
            'variable': sc.scalar(4.182, unit='s'),
            'data array': sc.DataArray(sc.scalar(52), coords={'x': sc.scalar('abc')}),
            'data group': sc.DataGroup(
                {
                    '1': sc.arange('x', 5),
                }
            ),
            'tråde': 'Ét eksempel',
            'a number': 123,
            'dict': {'k': 'value1', 'n': 77},
            'numpy-array': np.array([1, 6, 23]),
        }
    )
    dg2 = sc.DataGroup({key: dg1[key] for key in sorted(dg1.keys())})
    assert sc.identical(dg1, dg2)


def test_identical_data_group_mismatched_value() -> None:
    dg1 = sc.DataGroup(
        {
            'variable': sc.scalar(4.182, unit='s'),
            'data array': sc.DataArray(sc.scalar(52), coords={'x': sc.scalar('abc')}),
            'data group': sc.DataGroup(
                {
                    '1': sc.arange('x', 5),
                }
            ),
            'tråde': 'Ét eksempel',
            'a number': 123,
            'dict': {'k': 'value1', 'n': 77},
            'numpy-array': np.array([1, 6, 23]),
        }
    )

    dg2 = dg1.copy()
    dg2['variable'] = sc.scalar(512, unit='m')
    assert not sc.identical(dg1, dg2)

    dg2 = dg1.copy()
    dg2['data group'] = (
        sc.DataGroup(
            {
                '1': sc.arange('x', 5),
                '2': 'number 2',
            }
        ),
    )
    assert not sc.identical(dg1, dg2)

    dg2 = dg1.copy()
    dg2['trå'] = 'ein anderes beispiel'
    assert not sc.identical(dg1, dg2)


def test_identical_data_group_mismatched_type() -> None:
    dg1 = sc.DataGroup(
        {
            'variable': sc.scalar(4.182, unit='s'),
            'data array': sc.DataArray(sc.scalar(52), coords={'x': sc.scalar('abc')}),
            'data group': sc.DataGroup(
                {
                    '1': sc.arange('x', 5),
                }
            ),
            'tråde': 'Ét eksempel',
            'a number': 123,
            'dict': {'k': 'value1', 'n': 77},
            'numpy-array': np.array([1, 6, 23]),
        }
    )

    dg2 = dg1.copy()
    dg2['variable'] = np.array(4.182)
    assert not sc.identical(dg1, dg2)


def test_identical_data_group_different_keys() -> None:
    dg1 = sc.DataGroup(
        {
            'variable': sc.scalar(4.182, unit='s'),
            'data array': sc.DataArray(sc.scalar(52), coords={'x': sc.scalar('abc')}),
            'data group': sc.DataGroup(
                {
                    '1': sc.arange('x', 5),
                }
            ),
            'tråde': 'Ét eksempel',
            'a number': 123,
            'dict': {'k': 'value1', 'n': 77},
            'numpy-array': np.array([1, 6, 23]),
        }
    )

    dg2 = dg1.copy()
    del dg2['dict']
    assert not sc.identical(dg1, dg2)

    dg2 = dg1.copy()
    dg2['extra'] = sc.arange('t', 3)
    assert not sc.identical(dg1, dg2)


def test_identical_raises_TypeError_when_comparing_datagroup_to_Dataset() -> None:
    dg = sc.DataGroup(a=sc.scalar(1))
    ds = sc.Dataset({"a": sc.scalar(1)})
    with pytest.raises(TypeError):
        sc.identical(dg, ds)
    with pytest.raises(TypeError):
        sc.identical(ds, dg)


@pytest.fixture
def small():
    return sc.scalar(1.0)


@pytest.fixture
def medium():
    return sc.scalar(2.0)


@pytest.fixture
def large():
    return sc.scalar(3.0)


def test_eq(small, medium, large) -> None:
    assert not (small == medium).value
    assert not (small == medium.value).value
    assert not (small.values == medium).value

    assert (medium == medium).value
    assert (medium == medium.value).value
    assert (medium.value == medium).value

    assert not (large == medium).value
    assert not (large == medium.value).value
    assert not (large.value == medium).value


def test_ne(small, medium, large) -> None:
    assert (small != medium).value
    assert (small != medium.value).value
    assert (small.values != medium).value

    assert not (medium != medium).value
    assert not (medium != medium.value).value
    assert not (medium.value != medium).value

    assert (large != medium).value
    assert (large != medium.value).value
    assert (large.value != medium).value


def test_lt(small, medium, large) -> None:
    assert (small < medium).value
    assert (small < medium.value).value
    assert (small.values < medium).value

    assert not (medium < medium).value
    assert not (medium < medium.value).value
    assert not (medium.value < medium).value

    assert not (large < medium).value
    assert not (large < medium.value).value
    assert not (large.value < medium).value


def test_le(small, medium, large) -> None:
    assert (small <= medium).value
    assert (small <= medium.value).value
    assert (small.values <= medium).value

    assert (medium <= medium).value
    assert (medium <= medium.value).value
    assert (medium.value <= medium).value

    assert not (large <= medium).value
    assert not (large <= medium.value).value
    assert not (large.value <= medium).value


def test_gt(small, medium, large) -> None:
    assert not (small > medium).value
    assert not (small > medium.value).value
    assert not (small.values > medium).value

    assert not (medium > medium).value
    assert not (medium > medium.value).value
    assert not (medium.value > medium).value

    assert (large > medium).value
    assert (large > medium.value).value
    assert (large.value > medium).value


def test_ge(small, medium, large) -> None:
    assert not (small >= medium).value
    assert not (small >= medium.value).value
    assert not (small.values >= medium).value

    assert (medium >= medium).value
    assert (medium >= medium.value).value
    assert (medium.value >= medium).value

    assert (large >= medium).value
    assert (large >= medium.value).value
    assert (large.value >= medium).value


def test_comparison_with_str(medium) -> None:
    assert (sc.scalar('a string') == sc.scalar('a string')).value
    assert not (medium == 'a string')
    with pytest.raises(TypeError):
        _ = medium < 'a string'
