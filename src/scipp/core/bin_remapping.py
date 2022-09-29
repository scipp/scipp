# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from .._scipp import core as _cpp
from .cpp_classes import DataArray
from .like import empty_like
from .cumulative import cumsum

import time


def concat_bins(da, dim):
    start = time.time()
    out_dims = {d: s for d, s in da.sizes.items() if d != dim}
    # TODO masks

    sizes = da.bins.size().data
    print(time.time() - start)
    # subbin sizes
    # cumsum performed with the remove dim as innermost, such that we map all merged
    # bins into the same output bin
    sizes_sort_out = sizes.transpose(list(out_dims) + [dim])
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

    # TODO would drop empty bins, give groups explicitly!
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
                     coords={
                         name: coord
                         for name, coord in da.coords.items() if dim not in coord.dims
                     })
