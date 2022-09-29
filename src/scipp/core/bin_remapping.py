# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Callable, Dict, Literal, Optional, Union
import uuid

from .._scipp import core as _cpp
from .cpp_classes import DataArray
from .variable import empty, arange
from .like import empty_like
from .cumulative import cumsum

from math import prod
import time


def _bin_index(obj):
    nbin = prod(obj.shape)
    return arange('dummy', nbin).fold('dummy', sizes=obj.sizes)


def _input_bin_index(obj):
    return _bin_index(obj)


def _output_bin_index(obj, dim):
    sizes = dict(obj.sizes)
    del sizes[dim]
    return _bin_index(empty(dims=list(sizes.keys()),
                            shape=list(sizes.values()))).broadcast(dims=obj.dims,
                                                                   shape=obj.shape)


def concat_bins(da, dim):
    start = time.time()
    out_dims = {d: s for d, s in da.sizes.items() if d != dim}
    # TODO masks

    sizes = DataArray(
        da.bins.size().data,
        coords={
            'output_bin': _output_bin_index(da, dim),
            'input_bin': _input_bin_index(da),
        },
    )
    print(time.time() - start)
    sizes_sort_out = sizes.data.transpose(list(out_dims) + [dim])
    print(time.time() - start)

    # subbin sizes
    out_end = cumsum(sizes_sort_out)
    out_begin = out_end - sizes_sort_out
    print(time.time() - start)
    out = _cpp._bins_no_validate(
        data=empty_like(da.bins.constituents['data']),
        dim=da.bins.constituents['dim'],
        begin=out_begin.transpose(da.dims),
        end=out_end.transpose(da.dims),
    )
    print(time.time() - start)

    out[...] = da.data
    print(time.time() - start)

    #TODO would drop empty bins, give groups explicitly!
    out_sizes = sizes.sum(dim)
    out_end = cumsum(out_sizes.data)
    out_begin = out_end - out_sizes.data
    out = _cpp._bins_no_validate(
        data=out.bins.constituents['data'],
        dim=out.bins.constituents['dim'],
        begin=out_begin,
        end=out_end,
    )
    print(time.time() - start)
    return DataArray(out,
                     coords={
                         name: coord
                         for name, coord in da.coords.items() if dim not in coord.dims
                     })
