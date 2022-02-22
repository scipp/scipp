# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from typing import Optional, Sequence, Union

from hypothesis import strategies as st
from hypothesis.extra import numpy as npst

from ..core import DataArray, array


DIMS_ALPHABET = string.printable + 'µλÅΣx̅…'



def sizes(ndim: Optional[Union[int, st.SearchStrategy]] = None) -> st.SearchStrategy:
    if isinstance(ndim, st.SearchStrategy):
        return ndim.flatmap(lambda n: sizes(ndim=n))
    keys = dims()
    values = st.integers(min_value=1, max_value=10)
    if ndim is None:
        # The constructor of sc.Variable in Python only supports
        # arrays with <= 4 dimensions.
        return st.dictionaries(keys=keys, values=values, min_size=0, max_size=4)
    return st.dictionaries(keys=keys, values=values, min_size=ndim, max_size=ndim)


def units():
    return st.sampled_from(('one', 'm', 'kg', 's', 'A', 'K', 'count'))


def integer_dtypes(sizes: Sequence[int] = (32, 64)) -> st.SearchStrategy:
    return st.sampled_from([f'int{size}' for size in sizes])


def floating_dtypes(sizes: Sequence[int] = (32, 64)) -> st.SearchStrategy:
    return st.sampled_from([f'float{size}' for size in sizes])


def scalar_numeric_dtypes() -> st.SearchStrategy:
    return st.sampled_from((integer_dtypes, floating_dtypes)).flatmap(lambda f: f())


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
