# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

import scipp as sc

# to_unit has non-trivial logic to reduce the effect of rounding errors. The following
# test attempt to cover a wide range of potential unit scales in conversion to detect
# potential bugs.

_max_examples = 1000


@given(st.floats(min_value=1e-300, max_value=1e300))
@settings(max_examples=_max_examples)
def test_to_unit_value_1(x):
    unit = sc.Unit(f'{x:.14E}')
    var = sc.scalar(x, unit='')
    result = sc.to_unit(var, unit)
    assert result.unit == unit
    # to_unit is rounding to 1e-12 so generally we cannot expect more precision
    assert pytest.approx(result.value, abs=0.0, rel=1e-12) == 1.0


@given(st.floats(min_value=1e-300, max_value=1e300))
@settings(max_examples=_max_examples)
def test_to_unit_small_value(x):
    unit = sc.Unit(f'{x:.14E}')
    var = sc.scalar(1.2345e-6 * x, unit='')
    result = sc.to_unit(var, unit)
    assert result.unit == unit
    assert pytest.approx(result.value, abs=0.0, rel=1e-12) == 1.2345e-6


@given(st.floats(min_value=1e-300, max_value=1e300))
@settings(max_examples=_max_examples)
def test_to_unit_large_value(x):
    unit = sc.Unit(f'{x:.14E}')
    var = sc.scalar(1.2345e6 * x, unit='')
    result = sc.to_unit(var, unit)
    assert result.unit == unit
    assert pytest.approx(result.value, abs=0.0, rel=1e-12) == 1.2345e6
