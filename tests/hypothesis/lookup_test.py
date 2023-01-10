# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
from hypothesis import given
from hypothesis import strategies as st

import scipp as sc

float_args = dict(min_value=-1e300,
                  max_value=1e300,
                  allow_nan=False,
                  allow_infinity=False,
                  allow_subnormal=False)

# x = sc.linspace(dim='x', start=0.0, stop=1.0, num=4)
# vals = sc.array(dims=['x'], values=[3, 2, 1])
# hist = sc.DataArray(data=vals, coords={'x': x})
# sc.lookup(hist, 'x')[sc.array(dims=['event'], values=[0.1,0.4,0.1,0.6,0.9])]


@given(st.floats(**float_args), st.floats(**float_args))
def test_lookup_linspace_bounds(a, b):
    x = sc.array(dims=['row'], values=[a, b])
    upper = x.max()
    upper.value = np.nextafter(upper.value, np.inf)
    x_edges = sc.linspace('x', x.min(), upper, num=3)
    hist = sc.DataArray(sc.array(dims=['x'], values=[1.1, 1.1]), coords={'x': x_edges})
    func = sc.lookup(hist, 'x', fill_value=sc.scalar(1234.0))

    result = func(x)

    assert sc.identical(result, sc.full_like(x, value=1.1))


@given(st.floats(**float_args), st.floats(**float_args))
def test_lookup_scale_linspace_bounds(a, b):
    x = sc.array(dims=['row'], values=[a, b])
    upper = x.max()
    upper.value = np.nextafter(upper.value, np.inf)
    x_edges = sc.linspace('x', x.min(), upper, num=3)
    hist = sc.DataArray(sc.array(dims=['x'], values=[1.1, 1.1]), coords={'x': x_edges})
    func = sc.lookup(hist, 'x', fill_value=sc.scalar(1234.0))

    da = x.bin(x=x_edges[::2].copy())
    da.bins *= func

    assert da.sum().value == 2.2
