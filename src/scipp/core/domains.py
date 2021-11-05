# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

from . import DataArray, concat


def find_domains(da: DataArray) -> DataArray:
    """Find domains of constant values in a 1-D histogram.

    This effectively merges adjacent bins that have identical values.

    :param da: Input data array.
    :return: Data array with bins spanning domains of input.
    """
    dim = da.dim
    condition = da.data == da.data
    condition['x', :-1] = da.data['x', 1:] != da.data['x', :-1]
    condition
    group = f'{dim}_'
    tmp = DataArray(da.data, coords={dim: da.coords[dim][dim, 1:], group: condition})
    out = tmp.groupby(group).copy(1)
    del out.coords[group]
    out.coords[dim] = concat([da.coords[dim][dim, 0:1], out.coords[dim]], dim)
    return out
