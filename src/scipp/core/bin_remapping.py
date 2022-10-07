# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import uuid
from typing import Dict, List
from math import prod
from .._scipp import core as _cpp
from .cpp_classes import DataArray, Variable
from .cumulative import cumsum
from ..typing import Dims, VariableLikeType
from .variable import index
from .operations import where
from .concepts import (concrete_dims, reduced_coords, reduced_attrs, reduced_masks,
                       irreducible_mask)


def hide_masked_and_reduce_meta(da: DataArray, dim: Dims) -> DataArray:
    if (mask := irreducible_mask(da, dim)) is not None:
        # Avoid using boolean indexing since it would result in (partial) content
        # buffer copy. Instead index just begin/end and reuse content buffer.
        comps = da.bins.constituents
        if mask.ndim == 1:
            select = ~mask
            comps['begin'] = comps['begin'][select]
            comps['end'] = comps['end'][select]
        else:
            zero = index(0, dtype='int64')
            comps['begin'] = where(mask, zero, comps['begin'])
            comps['end'] = where(mask, zero, comps['end'])
        data = _cpp._bins_no_validate(**comps)
    else:
        data = da.data
    return DataArray(data,
                     coords=reduced_coords(da, dim),
                     masks=reduced_masks(da, dim),
                     attrs=reduced_attrs(da, dim))


def _with_bin_sizes(var: Variable, sizes: Variable) -> Variable:
    end = cumsum(sizes)
    begin = end - sizes
    data = var if var.bins is None else var.bins.constituents['data']
    dim = var.dim if var.bins is None else var.bins.constituents['dim']
    return _cpp._bins_no_validate(data=data, dim=dim, begin=begin, end=end)


def _sum(var: Variable, dims: List[str]) -> Variable:
    for dim in dims:
        var = var.sum(dim)
    return var


def _concat_bins(var: Variable, dims: List[str]) -> Variable:
    # To concat bins, two things need to happen:
    # 1. Data needs to be written to a contiguous chunk.
    # 2. New bin begin/end indices need to be setup.
    # If the dims to concatenate are the *inner* dims a call to `copy()` performs 1.
    # Otherwise, we first transpose and then `copy()`.
    # For step 2. we simply sum the (transposed) input bin sizes over the concat dims,
    # which `_with_bin_sizes` can use to compute new begin/end indices.
    changed_dims = dims
    unchanged_dims = [d for d in var.dims if d not in changed_dims]
    # TODO It would be possible to support a copy=False parameter, to skip the copy if
    # the copy would not result in an moving or reordering.
    out = var.transpose(unchanged_dims + changed_dims).copy()
    sizes = _sum(out.bins.size(), dims)
    return _with_bin_sizes(out, sizes)


def _combine_bins(var: Variable, coords: Dict[str, Variable], edges: List[Variable],
                  groups: List[Variable], erase: List[str]) -> Dict[str, Variable]:
    from .binning import make_binned
    # Overview
    # --------
    # The purpose of this code is to combine existing bins, but in a more general
    # manner than `concat`, which combines all bins along a dimension. Here we operate
    # more like `groupby`, which combines selected subset and creates a new output dim.
    #
    # Approach
    # --------
    # The algorithm works concetually similar to `_concat_bins`, but with an additional
    # step, calling `make_binned` for grouping within the erased dims. For the final
    # output binning, instead of summing the input bin sizes over all erased dims, we
    # sum only within the groups created by `make_binned`.

    # Preserve subspace dim order of input data, instead of the one given by `erase`
    changed_dims = [dim for dim in var.dims if dim in erase]
    unchanged_dims = [dim for dim in var.dims if dim not in changed_dims]
    changed_shape = [var.sizes[dim] for dim in changed_dims]
    unchanged_shape = [var.sizes[dim] for dim in unchanged_dims]
    changed_volume = prod(changed_shape)

    # Move modified dims to innermost to ensure data is written in contiguous memory.
    var = var.transpose(unchanged_dims + changed_dims)
    params = DataArray(var.bins.size(), coords=coords)
    params.attrs['begin'] = var.bins.constituents['begin'].copy()
    params.attrs['end'] = var.bins.constituents['end'].copy()

    # Sizes and begin/end indices of changed subspace
    sub_sizes = index(changed_volume).broadcast(dims=unchanged_dims,
                                                shape=unchanged_shape)
    params = params.flatten(to=uuid.uuid4().hex)
    params = _with_bin_sizes(params, sub_sizes)
    # Apply desired binning/grouping to sizes and begin/end
    params = make_binned(params, edges=edges, groups=groups)

    # Setup view of source content with desired target bin order
    source = _cpp._bins_no_validate(
        data=var.bins.constituents['data'],
        dim=var.bins.constituents['dim'],
        begin=params.bins.constituents['data'].attrs['begin'],
        end=params.bins.constituents['data'].attrs['end'],
    )
    # Call `copy()` to reorder data and `_with_bin_sizes` to put in place new indices.
    return _with_bin_sizes(source.copy(), sizes=params.data.bins.sum())


def combine_bins(da: DataArray, edges: List[Variable], groups: List[Variable],
                 erase: List[str]) -> DataArray:
    coords = {d: var for d, var in da.meta.items() if set(var.dims).issubset(erase)}
    da = hide_masked_and_reduce_meta(da, erase)
    if len(edges) == 0 and len(groups) == 0:
        data = _concat_bins(da.data, erase)
    else:
        data = _combine_bins(da.data,
                             coords=coords,
                             edges=edges,
                             groups=groups,
                             erase=erase)
    out = DataArray(data, coords=da.coords, masks=da.masks, attrs=da.attrs)
    for edge in edges:
        out.coords[edge.dim] = edge
    for group in groups:
        out.coords[group.dim] = group
    return out


def concat_bins(obj: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    erase = list(concrete_dims(obj, dim))
    da = obj if isinstance(obj, DataArray) else DataArray(obj)
    out = combine_bins(da, edges=[], groups=[], erase=erase)
    return out if isinstance(obj, DataArray) else out.data
