# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from hypothesis import given, settings
from hypothesis import strategies as st

import scipp as sc
import numpy as np

float_args = dict(min_value=-1e300,
                  max_value=1e300,
                  allow_nan=False,
                  allow_infinity=False,
                  allow_subnormal=False)


@given(st.floats(**float_args), st.floats(**float_args))
def test_bin_2d_linspace_bounds(a, b):
    x = sc.array(dims=['row'], values=[a, b])
    table = sc.DataArray(sc.ones(dims=['row'], shape=[2]))
    table.coords['x'] = x
    table.coords['y'] = x
    upper = x.max()
    upper.value = np.nextafter(upper.value, np.inf)
    x_edges = sc.linspace('x', x.min(), upper, num=3)
    y_edges = sc.linspace('y', x.min(), upper, num=3)
    binned = table.bin(x=x_edges, y=y_edges)
    assert binned.sum().value == 2


@settings(max_examples=1000)
@given(st.integers(min_value=0, max_value=5555), st.integers(min_value=0,
                                                             max_value=1234),
       st.integers(min_value=0, max_value=1234))
def test_automatic_grouping_optimization(nrow, i, j):
    base = sc.DataArray(sc.ones(dims=['row'], shape=[nrow]))
    base.coords['label'] = sc.arange('row', 0, nrow, dtype='int64')
    # Use overlapping or non-overlapping sections to simulate labels with duplicates as
    # well as gaps
    table = sc.concat([base[:i], base[j:]], 'row')
    assert sc.identical(
        table.group('label').coords['label'],
        sc.array(dims=['label'], values=np.unique(table.coords['label'].values)))
