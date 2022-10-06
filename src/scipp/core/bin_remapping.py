# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import uuid
from typing import Dict, List
from math import prod
from .._scipp import core as _cpp
from .cpp_classes import DataArray, Variable
from .cumulative import cumsum
from .dataset import irreducible_mask
from ..typing import VariableLikeType
from .variable import arange, full, index
from .operations import sort, where
from .comparison import identical
from .util import copy_for_overwrite
from .concepts import reduced_coords, reduced_attrs, reduced_masks


def hide_masked_and_reduce_meta(da: DataArray, dims: List[str]) -> DataArray:
    if len(dims) == 0:
        return da
    da = hide_masked_and_reduce_meta(da, dims[1:])
    dim = dims[0]
    if (mask := irreducible_mask(da.masks, dim)) is not None:
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


def _replace_bin_sizes(var: Variable, sizes: Variable) -> Variable:
    out_end = cumsum(sizes)
    out_begin = out_end - sizes
    return _cpp._bins_no_validate(
        data=var.bins.constituents['data'],
        dim=var.bins.constituents['dim'],
        begin=out_begin,
        end=out_end,
    )


def _combine_bins(var: Variable, begin, end, sizes) -> Variable:
    out = _cpp._bins_no_validate(
        data=copy_for_overwrite(var.bins.constituents['data']),
        dim=var.bins.constituents['dim'],
        begin=begin,
        end=end,
    )

    # Copy all bin contents, performing the actual reordering with the content buffer.
    out[...] = var

    # Setup output indices. This will have the "merged" bins, referencing the new
    # contiguous layout in the content buffer.
    return _replace_bin_sizes(out, sizes)


def _sum(var: Variable, dims: List[str]) -> Variable:
    for dim in dims:
        var = var.sum(dim)
    return var


def _concat_bins(var: Variable, dims: List[str]) -> Variable:
    # We want to write data from all bins along dim to a contiguous chunk in the
    # content buffer. This will then allow us to create new, larger bins covering the
    # respective input bins. We use `cumsum` after moving `dim` to the innermost dim.
    # This will allow us to setup offsets for the new contiguous layout.
    changed_dims = dims
    unchanged_dims = [d for d in var.dims if d not in changed_dims]
    out = var.transpose(unchanged_dims + changed_dims).copy()
    sizes = _sum(out.bins.size(), dims)
    return _replace_bin_sizes(out, sizes)


def _project(var: Variable, dim: str):
    while var.dims != (dim, ):
        outer = var.dims[0]
        min_ = var.min(outer)
        max_ = var.max(outer)
        assert identical(min_, max_)
        var = min_
    return var


def _setup_combine_bins_params(var: Variable, coords: Dict[str, Variable],
                               edges: List[Variable], groups: List[Variable],
                               erase: List[str]) -> Dict[str, Variable]:
    from .binning import make_binned
    # Overview
    # --------
    # The purpose of this code is to combine existing bins, but in a more general
    # manner than `concat`, which combines all bins along a dimension. Here we operate
    # more like `groupby`, which combines selected subset and creates a new output dim.
    #
    # Approach
    # --------
    # The dimensions of the input variable define an input space. The changed (erased)
    # dims define a subspace within that. For example, the input may have
    # dims=(x,y,z,t) and we combine bins in dims=(x,z), leaving y and t untouched.
    # The (x,z) subspace will be transformed into a new subspace with different
    # dimension labels and/or dimensionality. In this function we compute begin/end
    # offsets and final bin sizes that will be used by `_combine_bins` for reordering
    # data and seting up the output.
    # The algorithm steps are as follows:
    #
    # 1. The required parameters can be computed from the sizes of the input bins. We
    #    compute those and subsequently operate on the sizes.
    # 2. Move the changed subspace to innermost dims. This is ensures that 6.b yields
    #    offsets that result in correct data order.
    # 3. Flatten the changed subspace. The makes step 7. work.
    # 4. Setup pretend binning spanning the subspace. The result has only the dims of
    #    The unchanged subspace.
    # 5. Use `make_binned` to perform the "combine" step, yielding input bin sizes
    #    grouped by output bin.
    # 6. The result of 5.) provides two things:
    #    a. The bins.sum() yields output bin sizes.
    #    b. cumsum over the extracted buffer yields the input bin offset within the
    #       output content buffer.
    # 7. The result of 6.b is converted back to the input order, undoing the binning.

    # Preserve subspace dim order of input data, instead of the one given by `erase`
    changed_dims = [dim for dim in var.dims if dim in erase]
    unchanged_dims = [dim for dim in var.dims if dim not in changed_dims]
    sizes = DataArray(var.bins.size(), coords=coords)
    input_bin = uuid.uuid4().hex
    changed_shape = [var.sizes[dim] for dim in changed_dims]
    unchanged_shape = [var.sizes[dim] for dim in unchanged_dims]
    changed_volume = prod(changed_shape)
    # Move modified dims to innermost to ensure data is written in contiguous memory.
    sizes = sizes.transpose(unchanged_dims + changed_dims)
    # Flatten modified subspace for next steps. This is mainly necessary so we can
    # `sort` later to reshuffle back to input bin order, using the added `input_bin`
    # coord:
    flat_sizes = sizes.flatten(dims=changed_dims, to=input_bin)
    flat_sizes.coords[input_bin] = arange(input_bin, changed_volume, unit=None)

    # Sizes and begin/end indices of changed subspace
    sub_sizes = full(dims=unchanged_dims, shape=unchanged_shape, value=changed_volume)
    sub_end = cumsum(sub_sizes)
    sub_begin = sub_end - sub_sizes
    content_dim = uuid.uuid4().hex
    tmp = _cpp._bins_no_validate(data=flat_sizes.flatten(to=content_dim),
                                 dim=content_dim,
                                 begin=sub_begin,
                                 end=sub_end)
    tmp = make_binned(tmp, edges=edges, groups=groups)

    # As we started with a regular array of data we know that the result of merging the
    # bin contents is also regular, i.e., we can `fold` and then `sort`.
    end = cumsum(tmp.bins.concat().value).fold(dim=content_dim, sizes=flat_sizes.sizes)
    # This should be the same in every bin. Unfortunately the above process duplicates
    # it, so we have to project back to 1-D so `sort` works.
    end.coords[input_bin] = _project(end.coords[input_bin], input_bin)
    end = sort(end, input_bin).data
    end = end.fold(dim=input_bin, dims=changed_dims, shape=changed_shape)
    begin = end - sizes.data
    return {'begin': begin, 'end': end, 'sizes': tmp.data.bins.sum()}


def combine_bins(da: DataArray, edges: List[Variable], groups: List[Variable],
                 erase: List[str]) -> DataArray:
    coords = {
        d: coord
        for d, coord in da.coords.items() if set(coord.dims).issubset(erase)
    }
    da = hide_masked_and_reduce_meta(da, erase)
    if len(edges) == 0 and len(groups) == 0:
        data = _concat_bins(da.data, erase)
    else:
        params = _setup_combine_bins_params(da.data,
                                            coords=coords,
                                            edges=edges,
                                            groups=groups,
                                            erase=erase)
        data = _combine_bins(da.data, **params)
    out = DataArray(data, coords=da.coords, masks=da.masks, attrs=da.attrs)
    for edge in edges:
        out.coords[edge.dim] = edge
    for group in groups:
        out.coords[group.dim] = group
    return out


def concat_bins(obj: VariableLikeType, dim: str) -> VariableLikeType:
    da = obj if isinstance(obj, DataArray) else DataArray(obj)
    out = combine_bins(da, edges=[], groups=[], erase=[dim])
    return out if isinstance(obj, DataArray) else out.data
