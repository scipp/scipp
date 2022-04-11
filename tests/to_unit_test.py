# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
import scipp as sc


def test_to_unit():
    var = sc.scalar(1, unit='m')
    assert sc.to_unit(var, unit='mm').unit == sc.Unit('mm')
    assert sc.to_unit(var, unit=sc.Unit('mm')).unit == sc.Unit('mm')
    with pytest.raises(sc.UnitError):
        sc.to_unit(var, unit='abcdef')  # does not parse
    with pytest.raises(TypeError):
        sc.to_unit(var, unit=5)  # neither str nor Unit


@pytest.mark.parametrize('year', [1800, 1911, 1956, 1984, 2001, 2022, 2036, 2100, 2467])
def test_datetime64_with_explicit_ns_unit_is_consistent_irrespective_of_decimals(year):
    assert sc.identical(sc.datetime(f'{year}-01-01T12:00:00', unit='ns'),
                        sc.datetime(f'{year}-01-01T12:00:00.000', unit='ns'))
    assert sc.identical(sc.datetime(f'{year}-01-01T12:00:00', unit='ns'),
                        sc.datetime(f'{year}-01-01T12:00:00.000000', unit='ns'))
    assert sc.identical(sc.datetime(f'{year}-01-01T12:00:00', unit='ns'),
                        sc.datetime(f'{year}-01-01T12:00:00.000000000', unit='ns'))


@pytest.mark.parametrize('year', [1800, 1911, 1956, 1984, 2001, 2022, 2036, 2100, 2467])
def test_datetime64_to_ns_is_precise(year):
    assert sc.identical(
        sc.datetime(f'{year}-01-01T12:00:00', unit='s').to(unit='ns'),
        sc.datetime(f'{year}-01-01T12:00:00.000000000', unit='ns'))


def test_int64_seconds_to_ns_is_precise():
    assert sc.identical(
            sc.scalar(9223372036, unit='s').to(unit='ns'),
            sc.scalar(9223372036000000000, unit='ns'))
