# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Dict, Union
from .._scipp import core as _cpp
from .cpp_classes import DataArray, Variable, Dataset
from .like import empty_like
from .cumulative import cumsum
from .dataset import irreducible_mask

import time


def _reduced(obj: Dict[str, Variable], dim: str) -> Dict[str, Variable]:
    return {name: var for name, var in obj.items() if dim not in var.dims}


def reduced_coords(da: DataArray, dim: str) -> Dict[str, Variable]:
    return _reduced(da.coords, dim)


def reduced_attrs(da: DataArray, dim: str) -> Dict[str, Variable]:
    return _reduced(da.attrs, dim)


def reduced_masks(da: DataArray, dim: str) -> Dict[str, Variable]:
    return {name: mask.copy() for name, mask in _reduced(da.masks, dim).items()}


def _copy_dict_for_overwrite(mapping: Dict[str, Variable]):
    return {name: copy_for_overwrite(var) for name, var in mapping.items()}


def copy_for_overwrite(obj: Union[Variable, DataArray, Dataset]):
    """
    Copy a Scipp object for overwriting.

    Unlike :py:func:`scipp.empty_like` this does not preserve (and share) coord,
    mask, and attr values. Instead, those values are not initialized, just like the
    data values.
    """
    if isinstance(obj, Variable):
        return empty_like(obj)
    if isinstance(obj, DataArray):
        return DataArray(copy_for_overwrite(obj.data),
                         coords=_copy_dict_for_overwrite(obj.coords),
                         masks=_copy_dict_for_overwrite(obj.masks),
                         attrs=_copy_dict_for_overwrite(obj.attrs))


def hide_masked_and_reduce_meta(da: DataArray, dim: str):
    if (mask := irreducible_mask(da.masks, dim)) is not None:
        # Avoid using boolean indexing since it would result in (partial) content
        # buffer copy. Instead index just begin/end and reuse content buffer.
        comps = da.bins.constituents
        select = ~mask
        data = _cpp._bins_no_validate(
            data=comps['data'],
            dim=comps['dim'],
            begin=comps['begin'][select],
            end=comps['end'][select],
        )
    else:
        data = da.data
    return DataArray(data,
                     coords=reduced_coords(da, dim),
                     masks=reduced_masks(da, dim),
                     attrs=reduced_attrs(da, dim))


def concat_bins(da, dim):
    start = time.time()
    # TODO masks

    da = hide_masked_and_reduce_meta(da, dim)
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
        data=copy_for_overwrite(da.bins.constituents['data']),
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
    return DataArray(out, coords=da.coords, masks=da.masks, attrs=da.attrs)
