# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

from .._scipp.core import DataArray, DimensionError
from .shape import concat
from .variable import empty


def merge_equal_adjacent(da: DataArray) -> DataArray:
    """Merges adjacent bins that have identical values.

    Parameters
    ----------
    da:
        Input data array, must be a 1-D histogram.

    Returns
    -------
    :
        Data array with bins spanning domains of input.
    """
    try:
        dim = da.dim
    except DimensionError as e:
        raise DimensionError(
            "Cannot merge equal adjacent bins non-1-D data array"
        ) from e
    tmp = DataArray(da.data, coords={dim: da.coords[dim][dim, 1:]})
    condition = empty(dims=da.dims, shape=da.shape, dtype='bool')
    condition[dim, -1] = True
    condition[dim, :-1] = da.data[dim, 1:] != da.data[dim, :-1]
    out = tmp[condition]
    out.coords[dim] = concat([da.coords[dim][dim, 0:1], out.coords[dim]], dim)
    return out
