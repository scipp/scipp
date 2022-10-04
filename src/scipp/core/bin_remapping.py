# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import uuid
from typing import Dict, List
from math import prod
from .._scipp import core as _cpp
from .cpp_classes import DataArray, Variable, Dataset
from .like import empty_like
from .cumulative import cumsum
from .dataset import irreducible_mask
from ..typing import VariableLikeType
from .variable import arange, full
from .operations import sort
from .comparison import identical


def _reduced(obj: Dict[str, Variable], dim: str) -> Dict[str, Variable]:
    return {name: var for name, var in obj.items() if dim not in var.dims}


def reduced_coords(da: DataArray, dim: str) -> Dict[str, Variable]:
    return _reduced(da.coords, dim)


def reduced_attrs(da: DataArray, dim: str) -> Dict[str, Variable]:
    return _reduced(da.attrs, dim)


def reduced_masks(da: DataArray, dim: str) -> Dict[str, Variable]:
    return {name: mask.copy() for name, mask in _reduced(da.masks, dim).items()}


def _copy_dict_for_overwrite(mapping: Dict[str, Variable]) -> Dict[str, Variable]:
    return {name: copy_for_overwrite(var) for name, var in mapping.items()}


def copy_for_overwrite(obj: VariableLikeType) -> VariableLikeType:
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
    ds = Dataset(coords=_copy_dict_for_overwrite(obj.coords))
    for name, da in obj.items():
        ds[name] = DataArray(copy_for_overwrite(da.data),
                             masks=_copy_dict_for_overwrite(da.masks),
                             attrs=_copy_dict_for_overwrite(da.attrs))
    return ds


def hide_masked_and_reduce_meta(da: DataArray, dims: List[str]) -> DataArray:
    if len(dims) == 0:
        return da
    da = hide_masked_and_reduce_meta(da, dims[1:])
    dim = dims[0]
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


def _replace_bin_sizes(var: Variable, sizes: Variable) -> Variable:
    out_end = cumsum(sizes)
    out_begin = out_end - sizes
    return _cpp._bins_no_validate(
        data=var.bins.constituents['data'],
        dim=var.bins.constituents['dim'],
        begin=out_begin,
        end=out_end,
    )


def _remap_bins(var: Variable, begin, end, sizes) -> Variable:
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


def _concat_bins_variable(var: Variable, dim: str) -> Variable:
    # We want to write data from all bins along dim to a contiguous chunk in the
    # content buffer. This will then allow us to create new, larger bins covering the
    # respective input bins. We use `cumsum` after moving `dim` to the innermost dim.
    # This will allow us to setup offsets for the new contiguous layout.
    sizes = var.bins.size()
    out_dims = [d for d in var.dims if d != dim]
    out_end = cumsum(sizes.transpose(out_dims + [dim])).transpose(var.dims)
    out_begin = out_end - sizes
    out_sizes = sizes.sum(dim)
    return _remap_bins(var, out_begin, out_end, out_sizes)


def _project(var: Variable, dim: str):
    while var.dims != (dim, ):
        outer = var.dims[0]
        min_ = var.min(outer)
        max_ = var.max(outer)
        assert identical(min_, max_)
        var = min_
    return var


def func(var: Variable, coords, edges, groups, erase):
    """Given a number of input coords and output coords, map indices and sizes."""
    from .binning import make_binned
    # Preserve subspace dim order of input data, instead of using that given by `erase`
    changed_dims = [dim for dim in var.dims if dim in erase]
    unchanged_dims = [d for d in var.dims if d not in changed_dims]
    sizes = DataArray(var.bins.size(),
                      coords={
                          d: coord
                          for d, coord in coords.items()
                          if set(coord.dims).issubset(changed_dims)
                      })
    input_bin = uuid.uuid4().hex
    changed_shape = [var.sizes[dim] for dim in changed_dims]
    unchanged_shape = [var.sizes[dim] for dim in unchanged_dims]
    changed_volume = prod(changed_shape)
    # Move modified dims to innermost to ensure data is writen in contiguous memory.
    sizes = sizes.transpose(unchanged_dims + changed_dims)
    index_range = arange(input_bin, changed_volume, unit=None)
    # Flatten modified subspace for next steps. This is mainly necessary so we can
    # `sort` later to reshuffle back to input bin order.
    flat_sizes = sizes.flatten(dims=changed_dims, to=input_bin)
    flat_sizes.coords[input_bin] = index_range

    # Sizes and begin/end indices of changed subspace
    subspace_sizes = full(dims=unchanged_dims,
                          shape=unchanged_shape,
                          value=changed_volume)
    subspace_end = cumsum(subspace_sizes)
    subspace_begin = subspace_end - subspace_sizes
    content_dim = uuid.uuid4().hex
    tmp = _cpp._bins_no_validate(data=flat_sizes.flatten(to=content_dim),
                                 dim=content_dim,
                                 begin=subspace_begin,
                                 end=subspace_end)
    tmp = make_binned(DataArray(tmp), edges=edges, groups=groups, erase=changed_dims)

    # As we started with a regular array of data we know that the result of merging the
    # bin contents is also regular, i.e., we can `fold` and then `sort`.
    end = cumsum(tmp.bins.concat().value).fold(dim=content_dim, sizes=flat_sizes.sizes)
    # This should be the same in every bin. Unfortunately the above process duplicates
    # it, so we have to project back to 1-D so `sort` works.
    end.coords[input_bin] = _project(end.coords[input_bin], input_bin)
    out_end = sort(end, input_bin).fold(dim=input_bin,
                                        dims=changed_dims,
                                        shape=changed_shape).data
    out_begin = out_end - sizes.data
    return {'begin': out_begin, 'end': out_end, 'sizes': tmp.data.bins.sum()}


def remap_bins(da: DataArray, edges: List[Variable], groups: List[Variable],
               erase: List[str]) -> DataArray:
    coords = da.coords
    da = hide_masked_and_reduce_meta(da, erase)
    params = func(da.data, coords=coords, edges=edges, groups=groups, erase=erase)
    data = _remap_bins(da.data, **params)
    out = DataArray(data, coords=da.coords, masks=da.masks, attrs=da.attrs)
    for edge in edges:
        out.coords[edge.dim] = edge
    for group in groups:
        out.coords[group.dim] = group
    return out


def concat_bins(obj: VariableLikeType, dim: str) -> VariableLikeType:
    if isinstance(obj, Variable):
        return _concat_bins_variable(obj, dim)
    else:
        da = hide_masked_and_reduce_meta(obj, [dim])
        data = _concat_bins_variable(da.data, dim)
        return DataArray(data, coords=da.coords, masks=da.masks, attrs=da.attrs)
