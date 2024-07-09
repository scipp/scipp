# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

import scipp as sc

# to_unit has non-trivial logic to reduce the effect of rounding errors. The following
# test attempt to cover a wide range of potential unit scales in conversion to detect
# potential bugs.

_max_examples = 1000


@settings(max_examples=_max_examples, deadline=None)
def _to_unit_value_one(x):
    unit = sc.Unit(f'{x:.14E}')
    var = sc.scalar(x, unit='')
    result = sc.to_unit(var, unit)
    assert result.unit == unit
    # to_unit is rounding to 1e-12 so generally we cannot expect more precision
    assert result.value == pytest.approx(1.0, abs=0.0, rel=1.1e-12)


# - Current implementation shows rounding errors close to 1e-12 near x=1e-18
# - 1e-30 to 1e30 should cover most common region of unit scales
# - 1e-300 to 1e300 is for more extreme cases
test_to_unit_value_one_1 = given(st.floats(min_value=1e-19, max_value=1e-17))(
    _to_unit_value_one
)
test_to_unit_value_one_2 = given(st.floats(min_value=1e-30, max_value=1e30))(
    _to_unit_value_one
)
test_to_unit_value_one_3 = given(st.floats(min_value=1e-300, max_value=1e300))(
    _to_unit_value_one
)


@settings(max_examples=_max_examples, deadline=None)
def _to_unit_small_value(x):
    unit = sc.Unit(f'{x:.14E}')
    var = sc.scalar(1.2345e-6 * x, unit='')
    result = sc.to_unit(var, unit)
    assert result.unit == unit
    assert result.value == pytest.approx(1.2345e-6, abs=0.0, rel=1.1e-12)


test_to_unit_small_value_1 = given(st.floats(min_value=1e-19, max_value=1e-17))(
    _to_unit_small_value
)
test_to_unit_small_value_2 = given(st.floats(min_value=1e-30, max_value=1e30))(
    _to_unit_small_value
)
test_to_unit_small_value_3 = given(st.floats(min_value=1e-300, max_value=1e300))(
    _to_unit_small_value
)


@settings(max_examples=_max_examples, deadline=None)
def _to_unit_large_value(x):
    unit = sc.Unit(f'{x:.14E}')
    var = sc.scalar(1.2345e6 * x, unit='')
    result = sc.to_unit(var, unit)
    assert result.unit == unit
    assert result.value == pytest.approx(1.2345e6, abs=0.0, rel=1.1e-12)


test_to_unit_large_value_1 = given(st.floats(min_value=1e-19, max_value=1e-17))(
    _to_unit_large_value
)
test_to_unit_large_value_2 = given(st.floats(min_value=1e-30, max_value=1e30))(
    _to_unit_large_value
)
test_to_unit_large_value_3 = given(st.floats(min_value=1e-300, max_value=1e300))(
    _to_unit_large_value
)
