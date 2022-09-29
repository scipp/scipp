# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Callable, Dict, Literal, Optional, Union
import uuid

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLike, MetaDataMap
from .domains import merge_equal_adjacent
from .operations import islinspace
from .math import midpoints
from .shape import concat

from math import prod


def _bin_index(obj):
    nbin = prod(obj.shape)
    return sc.arange('dummy', nbin).fold('dummy', sizes=obj.sizes)


def _input_bin_index(obj, dim):
    return _bin_index(obj, dim)


def _output_bin_index(obj, dim):
    sizes = dict(obj.sizes)
    del sizes[dim]
    return _bin_index(sc.empty(dims=sizes.keys(),
                               shape=sizes.values())).broadcast(dims=obj.dims,
                                                                shape=obj.shape)


def concat_bins(da, dim):
    start = time.time()
    out_dims = {d: s for d, s in da.sizes.items() if d != dim}
    # TODO masks
    # output_bin_lut = sc.DataArray(sc.arange('param', len(edges)-1), coords={'param':edges})
    # func = sc.lookup(output_bin_lut)
    # output_bin = func(param)

    # sizes = sc.DataArray(da.bins.size().data, coords={'output_bin':output_bin, 'input_bin':sc.arange('x', len(da))})

    sizes = sc.DataArray(
        da.bins.size().data,
        coords={
            'output_bin': output_bin_index(da, dim),
            'input_bin': input_bin_index(da),
        },
    )
    print(time.time() - start)
    #sizes2 = sizes.flatten(to='dummy')
    #sizes_sort_out2 = sc.sort(sizes2, 'output_bin')
    sizes_sort_out = sizes.transpose(list(out_dims) + [dim])
    print(time.time() - start)

    # subbin sizes
    out_end = sc.cumsum(sizes_sort_out.data)
    out_begin = out_end - sizes_sort_out.data
    sizes_sort_out.coords['out_begin'] = out_begin
    sizes_sort_out.coords['out_end'] = out_end
    #sizes_sort_in = sc.sort(sizes_sort_out, 'input_bin')
    sizes_sort_in = sizes_sort_out.transpose(da.dims)
    print(time.time() - start)
    out = _cpp._bins_no_validate(
        data=sc.empty_like(da.bins.constituents['data']),
        dim=da.bins.constituents['dim'],
        begin=sizes_sort_in.coords['out_begin'].transpose(da.dims),
        end=sizes_sort_in.coords['out_end'].transpose(da.dims),
    )
    print(time.time() - start)

    #out[...] = da.data.flatten(to='dummy')
    out[...] = da.data
    print(time.time() - start)

    #TODO would drop empty bins, give groups explicitly!
    #out_sizes = sizes_sort_out.groupby('output_bin').sum('dummy').fold('output_bin', sizes=out_dims)
    out_sizes = sizes_sort_out.sum(dim)
    out_end = sc.cumsum(out_sizes.data)
    out_begin = out_end - out_sizes.data
    out = sc.bins(
        data=out.bins.constituents['data'],
        dim=out.bins.constituents['dim'],
        begin=out_begin,
        end=out_end,
    )
    print(time.time() - start)
    out = sc.DataArray(out,
                       coords={
                           name: coord
                           for name, coord in da.coords.items() if dim not in coord.dims
                       })
    return out
