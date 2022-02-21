# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

import string

from hypothesis import strategies as st
from hypothesis.extra import numpy as npst

from ..core import DataArray, array


def dims():
    return st.text(alphabet=string.printable, min_size=1, max_size=10)


def sizes(ndim=None):
    keys = dims()
    values = st.integers(min_value=1, max_value=10)
    if ndim is None:
        # The constructor of sc.Variable in Python only supports
        # arrays with <= 4 dimensions.
        return st.dictionaries(keys=keys, values=values, min_size=1, max_size=4)
    return st.dictionaries(keys=keys, values=values, min_size=ndim, max_size=ndim)


def units():
    return st.sampled_from(('one', 'm', 'kg', 's', 'A', 'K', 'count'))


def dtypes():
    return st.sampled_from((int, float))


@st.composite
def fixed_variables(draw, dtype, sizes):
    values = draw(npst.arrays(dtype, shape=tuple(sizes.values())))
    if dtype == float and draw(st.booleans()):
        variances = draw(npst.arrays(dtype, shape=values.shape))
    else:
        variances = None
    dims = list(sizes.keys())
    unit = draw(units())
    return array(dims=dims, values=values, variances=variances, unit=unit)


@st.composite
def variables(draw, dtype=None, ndim=None):
    if dtype is None:
        dtype = draw(dtypes())
    return draw(sizes(ndim).flatmap(lambda s: fixed_variables(dtype=dtype, sizes=s)))


@st.composite
def coord_dicts_1d(draw, sizes):
    return {
        dim: draw(fixed_variables(dtype=int, sizes={dim: size}))
        for dim, size in sizes.items()
    }


@st.composite
def dataarrays(draw):
    data = draw(variables(dtype=float))
    coords = draw(coord_dicts_1d(sizes=data.sizes))
    return DataArray(data, coords=coords)
