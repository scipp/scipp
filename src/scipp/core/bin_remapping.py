# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Dict
from .._scipp import core as _cpp
from .cpp_classes import DataArray, Variable
from .like import empty_like
from .cumulative import cumsum

import time


def _reduced(obj: Dict[str, Variable], dim: str) -> Dict[str, Variable]:
    return {name: var for name, var in obj.items() if dim not in var.dims}


def reduced_coords(da: DataArray, dim: str) -> Dict[str, Variable]:
    return _reduced(da.coords, dim)


def reduced_attrs(da: DataArray, dim: str) -> Dict[str, Variable]:
    return _reduced(da.attrs, dim)


def reduced_masks(da: DataArray, dim: str) -> Dict[str, Variable]:
    return _reduced(da.masks, dim)


def concat_bins(da, dim):
    start = time.time()
    # TODO masks

    sizes = da.data.bins.size()
    print(time.time() - start)
    # subbin sizes
    # cumsum performed with the remove dim as innermost, such that we map all merged
    # bins into the same output bin
    out_dims = [d for d in da.dims if d != dim]
    out_end = cumsum(sizes.transpose(out_dims + [dim])).transpose(da.dims)
    out_begin = out_end - sizes
    print(time.time() - start)
    out = _cpp._bins_no_validate(
        data=empty_like(da.bins.constituents['data']),
        dim=da.bins.constituents['dim'],
        begin=out_begin,
        end=out_end,
    )
    print(time.time() - start)

    out[...] = da.data
    print(time.time() - start)

    out_sizes = sizes.sum(dim)
    out_end = cumsum(out_sizes)
    out_begin = out_end - out_sizes
    out = _cpp._bins_no_validate(
        data=out.bins.constituents['data'],
        dim=out.bins.constituents['dim'],
        begin=out_begin,
        end=out_end,
    )
    print(time.time() - start)
    return DataArray(out,
                     coords=reduced_coords(da, dim),
                     masks=reduced_masks(da, dim),
                     attrs=reduced_attrs(da, dim))
