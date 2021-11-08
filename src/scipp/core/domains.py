# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

from .._scipp.core import DataArray, DimensionError
from .shape import concat
from .variable import empty


def merge_equal_adjacent(da: DataArray) -> DataArray:
    """Merges adjacent bins that have identical values.

    :param da: Input data array, must be a 1-D histogram.
    :return: Data array with bins spanning domains of input.
    """
    try:
        dim = da.dim
    except DimensionError as e:
        raise DimensionError(
            "Cannot merge equal adjacent bins non-1-D data array") from e
    condition = empty(dims=da.dims, shape=da.shape, dtype='bool')
    condition[dim, -1] = False
    condition[dim, :-1] = da.data[dim, 1:] == da.data[dim, :-1]
    group = f'{dim}_'
    tmp = DataArray(da.data, coords={dim: da.coords[dim][dim, 1:], group: condition})
    # Note the flipped condition: In case we do not merge any bins we may have only a
    # single group. The flipped condition ensures that we always require group 0.
    out = tmp.groupby(group).copy(0)
    del out.coords[group]
    out.coords[dim] = concat([da.coords[dim][dim, 0:1], out.coords[dim]], dim)
    return out
