# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import itertools
from collections.abc import Sequence
from math import prod
from typing import TypeVar

from .._scipp import core as _cpp
from ..typing import Dims
from .concepts import concrete_dims, irreducible_mask, rewrap_reduced_data
from .cpp_classes import DataArray, Variable
from .cumulative import cumsum
from .operations import where
from .variable import index


def hide_masked(da: DataArray, dim: Dims) -> Variable:
    if not da.is_binned:
        raise ValueError("Input must be binned")
    if (mask := irreducible_mask(da, dim)) is not None:
        # Avoid using boolean indexing since it would result in (partial) content
        # buffer copy. Instead index just begin/end and reuse content buffer.
        comps = da.bins.constituents
        # If the mask is 1-D we can drop entire "rows" or "columns". This can
        # drastically reduce the number of bins to handle in some cases for better
        # performance. For 2-D or higher masks we fall back to making bins "empty" by
        # setting end=begin.
        if mask.ndim == 1:
            select = ~mask
            comps['begin'] = comps['begin'][select]
            comps['end'] = comps['end'][select]
        else:
            comps['end'] = where(mask, comps['begin'], comps['end'])
        return _cpp._bins_no_validate(**comps)  # type: ignore[no-any-return]
    else:
        return da.data


def _with_bin_sizes(var: Variable | DataArray, sizes: Variable) -> Variable:
    end = cumsum(sizes)
    begin = end - sizes
    data = var.bins.constituents['data'] if var.is_binned else var
    dim = var.bins.constituents['dim'] if var.is_binned else var.dim
    return _cpp._bins_no_validate(data=data, dim=dim, begin=begin, end=end)  # type: ignore[no-any-return]


def _concat_bins(var: Variable, dim: Dims) -> Variable:
    # To concat bins, two things need to happen:
    # 1. Data needs to be written to a contiguous chunk.
    # 2. New bin begin/end indices need to be setup.
    # If the dims to concatenate are the *inner* dims a call to `copy()` performs 1.
    # Otherwise, we first transpose and then `copy()`.
    # For step 2. we simply sum the (transposed) input bin sizes over the concat dims,
    # which `_with_bin_sizes` can use to compute new begin/end indices.
    changed_dims = list(concrete_dims(var, dim))
    unchanged_dims = [d for d in var.dims if d not in changed_dims]
    # TODO It would be possible to support a copy=False parameter, to skip the copy if
    # the copy would not result in any moving or reordering.
    out = var.transpose(unchanged_dims + changed_dims).copy()
    out_bins = out.bins
    sizes = out_bins.size().sum(changed_dims)
    return _with_bin_sizes(out, sizes)


def _combine_bins(
    var: Variable,
    coords: dict[str, Variable],
    edges: Sequence[Variable],
    groups: Sequence[Variable],
    dim: Dims,
) -> Variable:
    from .binning import make_binned

    # Overview
    # --------
    # The purpose of this code is to combine existing bins, but in a more general
    # manner than `concat`, which combines all bins along a dimension. Here we operate
    # more like `groupby`, which combines selected subsets and creates a new output dim.
    #
    # Approach
    # --------
    # The algorithm works conceptually similar to `_concat_bins`, but with an additional
    # step, calling `make_binned` for grouping within the erased dims. For the final
    # output binning, instead of summing the input bin sizes over all erased dims, we
    # sum only within the groups created by `make_binned`.
    # Preserve subspace dim order of input data, instead of the one given by `dim`
    concrete_dims_ = concrete_dims(var, dim)
    changed_dims = [d for d in var.dims if d in concrete_dims_]
    unchanged_dims = [d for d in var.dims if d not in changed_dims]
    changed_shape = [var.sizes[d] for d in changed_dims]
    unchanged_shape = [var.sizes[d] for d in unchanged_dims]
    changed_volume = prod(changed_shape)

    # Move modified dims to innermost. Below this enables us to keep other dims
    # (listed in unchanged_dims) untouched by creating pseudo bins that wrap the entire
    # changed subspaces. make_binned below will thus only operate within each pseudo
    # bins, without mixing contents from different unchanged bins.
    var = var.transpose(unchanged_dims + changed_dims)
    var_bins = var.bins
    params = DataArray(var_bins.size(), coords=coords)
    params.coords['begin'] = var_bins.constituents['begin'].copy()
    params.coords['end'] = var_bins.constituents['end'].copy()

    # Sizes and begin/end indices of changed subspace
    sub_sizes = index(changed_volume).broadcast(
        dims=unchanged_dims, shape=unchanged_shape
    )
    params = params.flatten(to="_combine_bins.flat_dim")
    # Setup pseudo binning for unchanged subspace. All further reordering (for grouping
    # and binning) will then occur *within* those pseudo bins (by splitting them).
    params_data = _with_bin_sizes(params, sub_sizes)
    # Apply desired binning/grouping to sizes and begin/end, splitting the pseudo bins.
    params = make_binned(params_data, edges=edges, groups=groups)

    # Setup view of source content with desired target bin order
    source = _cpp._bins_no_validate(
        data=var_bins.constituents['data'],
        dim=var_bins.constituents['dim'],
        begin=params.bins.constituents['data'].coords['begin'],  # type: ignore[union-attr]
        end=params.bins.constituents['data'].coords['end'],  # type: ignore[union-attr]
    )
    # Call `copy()` to reorder data. This is based on the underlying behavior of `copy`
    # for binned data: It computes a new contiguous and ordered mapping of bin contents
    # to the content buffer. The main purpose of that mechanism is to deal, e.g., with
    # copies of slices, but here we can leverage the same mechanism.
    # Then we call `_with_bin_sizes` to put in place new indices, "merging" the
    # reordered input bins to desired output bins.
    return _with_bin_sizes(source.copy(), sizes=params.data.bins.sum())


def combine_bins(
    da: DataArray, edges: Sequence[Variable], groups: Sequence[Variable], dim: Dims
) -> DataArray:
    if not da.is_binned:
        raise ValueError("Input must be binned")
    masked = hide_masked(da, dim)
    if len(edges) == 0 and len(groups) == 0:
        data = _concat_bins(masked, dim=dim)
    else:
        names = [coord.dim for coord in itertools.chain(edges, groups)]
        coords = {name: da.coords[name] for name in names}
        data = _combine_bins(masked, coords=coords, edges=edges, groups=groups, dim=dim)
    out = rewrap_reduced_data(da, data, dim=dim)
    for coord in itertools.chain(edges, groups):
        out.coords[coord.dim] = coord
    return out


_VarDa = TypeVar('_VarDa', Variable, DataArray)


def concat_bins(obj: _VarDa, dim: Dims = None) -> _VarDa:
    da = obj if isinstance(obj, DataArray) else DataArray(obj)  # type: ignore[redundant-expr]
    out = combine_bins(da, edges=[], groups=[], dim=dim)
    return out if isinstance(obj, DataArray) else out.data  # type: ignore[redundant-expr]
