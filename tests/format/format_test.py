# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Gregory Tucker, Jan-Lukas Wynen

import numpy as np
import pytest

import scipp as sc


@pytest.mark.parametrize(
    'var',
    [
        sc.scalar(1),
        sc.scalar(-4, dtype='int32'),
        sc.scalar(3.1, variance=0.1, unit='m'),
        sc.scalar(2.1, variance=0.1, dtype='float32'),
        sc.array(dims=['x', 't'], values=np.ones((3, 4)), unit='kg/s'),
        sc.scalar('some string'),
        sc.array(dims=['s'], values=['str', '2']),
        sc.scalar(6134, dtype='datetime64', unit='s'),
        sc.array(dims=['e'], values=[512, 1662], unit='s'),
    ],
)
def test_variable_default(var: sc.Variable) -> None:
    assert f'{var}' == str(var)
    assert f'{var:}' == str(var)
    assert f'{var::}' == str(var)
    assert '{:}'.format(var) == str(var)  # noqa: UP032, RUF100


@pytest.mark.parametrize('s', ['^', ''])
def test_variable_default_length_central(s: str) -> None:
    var = sc.arange('x', 10)
    assert '[0, 1, ..., 8, 9]' in f'{var:{s}}'
    assert '[0, 1, ..., 8, 9]' in f'{var:{s}#4}'
    assert '[0, 1, ..., 7, 8, 9]' in f'{var:{s}#5}'
    assert '[0, 1, 2, ..., 7, 8, 9]' in f'{var:{s}#6}'

    var = sc.arange('x', 4)
    assert '[0, 1, 2, 3]' in f'{var:{s}}'
    assert '[0, 1, 2, 3]' in f'{var:{s}#5}'
    assert '[0, ..., 2, 3]' in f'{var:{s}#3}'
    assert '[0, ..., 3]' in f'{var:{s}#2}'
    assert '[..., 3]' in f'{var:{s}#1}'
    assert '[...]' in f'{var:{s}#0}'

    var = sc.arange('x', 0)
    assert '[]' in f'{var:{s}}'
    assert '[]' in f'{var:{s}#6}'
    assert '[]' in f'{var:{s}#0}'

    var = sc.scalar(31354)
    assert '31354' in f'{var:{s}}'
    assert '31354' in f'{var:{s}#2}'
    assert '31354' not in f'{var:{s}#0}'
    assert '...' in f'{var:{s}#0}'


def test_variable_default_length_left() -> None:
    var = sc.arange('x', 10)
    assert '[0, 1, 2, 3, ...]' in f'{var:<}'
    assert '[0, 1, 2, 3, ...]' in f'{var:<#4}'
    assert '[0, 1, 2, 3, 4, ...]' in f'{var:<#5}'
    assert '[0, 1, 2, 3, 4, 5, ...]' in f'{var:<#6}'

    var = sc.arange('x', 4)
    assert '[0, 1, 2, 3]' in f'{var:<}'
    assert '[0, 1, 2, 3]' in f'{var:<#5}'
    assert '[0, 1, 2, ...]' in f'{var:<#3}'
    assert '[0, 1, ...]' in f'{var:<#2}'
    assert '[0, ...]' in f'{var:<#1}'
    assert '[...]' in f'{var:<#0}'

    var = sc.arange('x', 0)
    assert '[]' in f'{var:<}'
    assert '[]' in f'{var:<#6}'
    assert '[]' in f'{var:<#0}'

    var = sc.scalar(4512)
    assert '4512' in f'{var:<}'
    assert '4512' in f'{var:<#2}'
    assert '4512' not in f'{var:<#0}'
    assert '...' in f'{var:<#0}'


def test_variable_default_length_right() -> None:
    var = sc.arange('x', 10)
    assert '[..., 6, 7, 8, 9]' in f'{var:>}'
    assert '[..., 6, 7, 8, 9]' in f'{var:>#4}'
    assert '[..., 5, 6, 7, 8, 9]' in f'{var:>#5}'
    assert '[..., 4, 5, 6, 7, 8, 9]' in f'{var:>#6}'

    var = sc.arange('x', 4)
    assert '[0, 1, 2, 3]' in f'{var:>}'
    assert '[0, 1, 2, 3]' in f'{var:>#5}'
    assert '[..., 1, 2, 3]' in f'{var:>#3}'
    assert '[..., 2, 3]' in f'{var:>#2}'
    assert '[..., 3]' in f'{var:>#1}'
    assert '[...]' in f'{var:>#0}'

    var = sc.arange('x', 0)
    assert '[]' in f'{var:>}'
    assert '[]' in f'{var:>#6}'
    assert '[]' in f'{var:>#0}'

    var = sc.scalar(846)
    assert '846' in f'{var:>}'
    assert '846' in f'{var:>#2}'
    assert '846' not in f'{var:>#0}'
    assert '...' in f'{var:>#0}'


def test_variable_default_variances() -> None:
    var = sc.arange('x', 10.0)
    var.variances = var.values
    var.values *= -1
    assert f'{var}' == str(var)
    # matches array of variances
    assert '[0, 1, ..., 8, 9]' in f'{var}'
    assert '[0, 1, 2, 3, 4, ...]' in f'{var:<#5}'


def test_variable_default_nested_exponential() -> None:
    var = sc.array(dims=['ys'], values=[1.2345, 654.98], unit='kg')
    res = f'{var::.2e}'
    assert f'{1.2345:.2e}' in res
    assert f'{654.98:.2e}' in res


def test_variable_default_forwards_to_nested_scalar() -> None:
    class C:
        def __format__(self, format_spec: str) -> str:
            return f'NESTED-{format_spec}'

    var = sc.scalar(C())
    assert 'NESTED-abcd#0' in f'{var::abcd#0}'


def test_variable_compact_scalar_no_variance() -> None:
    var = sc.scalar(100, unit='s')
    assert f'{var:c}' == '100 s'


def test_variable_compact_scalar_with_variance() -> None:
    scalar_variables = [
        (100.0, 1.0, 'm', '100.0(10) m'),
        (100.0, 2.0, '1', '100(2)'),
        (100.0, 10.0, 'counts', '100(10) counts'),
        (100.0, 100.0, 'us', '100(100) µs'),
        (0.01, 0.001, 'angstrom', '0.0100(10) Å'),
        (0.01, 0.002, 'cm', '0.010(2) cm'),
        (np.pi, 0.00003, 'rad', '3.14159(3) rad'),
        # default rounding rules for half:
        (234.567, 1.25, 'km', '234.6(12) km'),  # even + 0.5 -> even
        (234.567, 1.35, 'km', '234.6(14) km'),  # odd + 0.5 -> even (odd + 1)
        # zero variance should be treated like None
        # ideally we want to use integer value and variance to avoid fragility
        # in the output of, e.g., str(100.), but scipp does not allow dtype=int64
        # with a specified variance
        (100.0, 0.0, 'C', '100.0 C'),
    ]
    for value, error, unit, expected in scalar_variables:
        var = sc.scalar(value, variance=error**2, unit=unit)
        assert f'{var:c}' == expected


def test_variable_compact_array_no_variance() -> None:
    var = sc.array(dims=['fg'], values=[100, 20, 3], unit='s')
    assert f'{var:c}' == '100, 20, 3 s'


def test_variable_compact_array_with_variance() -> None:
    array_variables = [
        ([100.0, 20.0], [1.0, 2.0], 'm', '100.0(10), 20(2) m'),
        (
            [9000.0, 800.0, 70.0, 6.0],
            [100.0, 20.0, 3.0, 0.4],
            '1',
            '9000(100), 800(20), 70(3), 6.0(4)',
        ),
        ([1.0, 2.0, 3.0], [0.0, 1.0, 0.2], 'C', '1.0, 2.0(10), 3.0(2) C'),
    ]
    for values, errors, unit, expected in array_variables:
        var = sc.array(
            dims=['ga'], values=values, variances=np.array(errors) ** 2, unit=unit
        )
        assert f'{var:c}' == expected


def test_variable_compact_raises_for_length() -> None:
    var = sc.scalar(2)
    with pytest.raises(ValueError, match='Invalid format spec'):
        f'{var:#3c}'
    with pytest.raises(ValueError, match='Invalid format spec'):
        f'{var:#4c}'
    with pytest.raises(ValueError, match='Invalid format spec'):
        f'{var:#6c}'


def test_variable_compact_raises_for_selection() -> None:
    var = sc.scalar(2)
    with pytest.raises(ValueError, match='Invalid format spec'):
        f'{var:<c}'
    with pytest.raises(ValueError, match='Invalid format spec'):
        f'{var:>c}'
    with pytest.raises(ValueError, match='Invalid format spec'):
        f'{var:^c}'


def test_variable_compact_raises_for_nested() -> None:
    var = sc.scalar(2)
    with pytest.raises(ValueError, match='Invalid format spec'):
        f'{var:c:f}'


def test_variable_compact_only_supports_numeric_dtype() -> None:
    var = sc.scalar('a string')
    with pytest.raises(ValueError, match='Compact formatting.*string'):
        f'{var:c}'
