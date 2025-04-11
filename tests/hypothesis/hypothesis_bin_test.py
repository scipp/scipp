# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import itertools

import numpy as np
from hypothesis import given, settings
from hypothesis import strategies as st

import scipp as sc
from scipp.testing import strategies as scst

float_args = {
    'min_value': -1e300,
    'max_value': 1e300,
    'allow_nan': False,
    'allow_infinity': False,
    'allow_subnormal': False,
}


@given(st.floats(**float_args), st.floats(**float_args))
def test_bin_2d_linspace_bounds(a, b) -> None:
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


@settings(max_examples=1000, deadline=1000)
@given(
    st.integers(min_value=0, max_value=5555),
    st.integers(min_value=0, max_value=1234),
    st.integers(min_value=0, max_value=1234),
)
def test_automatic_grouping_optimization(nrow, i, j) -> None:
    base = sc.DataArray(sc.ones(dims=['row'], shape=[nrow]))
    base.coords['label'] = sc.arange('row', 0, nrow, dtype='int64')
    # Use overlapping or non-overlapping sections to simulate labels with duplicates as
    # well as gaps
    table = sc.concat([base[:i], base[j:]], 'row')
    assert sc.identical(
        table.group('label').coords['label'],
        sc.array(dims=['label'], values=np.unique(table.coords['label'].values)),
    )


@settings(max_examples=100, deadline=1000)
@given(
    st.integers(min_value=1, max_value=12345),
    scst.variables(dtype=bool, ndim=st.integers(min_value=1, max_value=3)),
)
def test_bins_concat_masking(nevent, mask) -> None:
    from numpy.random import default_rng

    rng = default_rng(seed=1234)
    mask.unit = None
    table = sc.DataArray(sc.arange('row', nevent))
    for dim in mask.dims:
        table.coords[dim] = sc.array(dims=['row'], values=rng.random((nevent,)))
    da = table.bin(mask.sizes)
    for n in range(1, mask.ndim + 1):
        for dims in itertools.permutations(mask.dims, n):
            m = mask
            for dim in mask.dims:
                if dim not in dims:
                    m = m[dim, 0]
            da.masks["".join(dims)] = m
    for n in range(1, da.ndim + 1):
        for dims in itertools.permutations(da.dims, n):
            assert sc.identical(da.bins.concat(dims).hist(), da.hist().sum(dims))
            assert sc.identical(
                da.transpose().bins.concat(dims).hist(), da.transpose().hist().sum(dims)
            )
    assert sc.identical(da.bins.concat(None).hist(), da.hist().sum(None))
