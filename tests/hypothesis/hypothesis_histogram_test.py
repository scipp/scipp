# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
from hypothesis import given
from hypothesis import strategies as st

import scipp as sc

float_args = {
    'min_value': -1e300,
    'max_value': 1e300,
    'allow_nan': False,
    'allow_infinity': False,
    'allow_subnormal': False,
}


@given(st.floats(**float_args), st.floats(**float_args))
def test_histogram_linspace_bounds(a, b) -> None:
    x = sc.array(dims=['row'], values=[a, b])
    table = sc.DataArray(sc.ones(dims=['row'], shape=[2]))
    table.coords['x'] = x
    upper = x.max()
    upper.value = np.nextafter(upper.value, np.inf)
    edges = sc.linspace('x', x.min(), upper, num=3)
    hist = table.hist(x=edges)
    assert hist.sum().value == 2
