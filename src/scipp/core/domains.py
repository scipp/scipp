# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

from .._scipp.core import DataArray
from .shape import concat


def find_domains(da: DataArray) -> DataArray:
    """Find domains of constant values in a 1-D histogram.

    This effectively merges adjacent bins that have identical values.

    :param da: Input data array.
    :return: Data array with bins spanning domains of input.
    """
    dim = da.dim
    condition = da.data != da.data
    condition[dim, :-1] = da.data[dim, 1:] == da.data[dim, :-1]
    group = f'{dim}_'
    tmp = DataArray(da.data, coords={dim: da.coords[dim][dim, 1:], group: condition})
    # Note the flipped condition: In case we do not merge any bins we may have only a
    # single group. The flipped condition ensures that we always require group 0.
    out = tmp.groupby(group).copy(0)
    del out.coords[group]
    out.coords[dim] = concat([da.coords[dim][dim, 0:1], out.coords[dim]], dim)
    return out
