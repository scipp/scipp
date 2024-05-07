# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from collections.abc import Callable, Mapping
from functools import reduce

from ..typing import Dims, VariableLikeType
from .cpp_classes import DataArray, Variable
from .logical import logical_or


def _copied(obj: Mapping[str, Variable]) -> dict[str, Variable]:
    return {name: var.copy() for name, var in obj.items()}


def _reduced(obj: Mapping[str, Variable], dims: list[str]) -> dict[str, Variable]:
    dims = set(dims)
    return {name: var for name, var in obj.items() if dims.isdisjoint(var.dims)}


def rewrap_output_data(prototype: VariableLikeType, data) -> VariableLikeType:
    if isinstance(prototype, DataArray):
        return DataArray(
            data=data,
            coords=prototype.coords,
            attrs=prototype.deprecated_attrs,
            masks=_copied(prototype.masks),
        )
    else:
        return data


def rewrap_reduced_data(
    prototype: VariableLikeType, data, dim: Dims
) -> VariableLikeType:
    return DataArray(
        data,
        coords=reduced_coords(prototype, dim),
        masks=reduced_masks(prototype, dim),
        attrs=reduced_attrs(prototype, dim),
    )


def transform_data(obj: VariableLikeType, func: Callable) -> VariableLikeType:
    if isinstance(obj, Variable):
        return func(obj)
    if isinstance(obj, DataArray):
        return rewrap_output_data(obj, func(obj.data))
    else:
        raise TypeError(f"{func} only supports Variable and DataArray as inputs.")


def concrete_dims(obj: VariableLikeType, dim: Dims) -> tuple[str]:
    """Convert a dimension specification into a concrete tuple of dimension labels.

    This does *not* validate that the dimension labels are valid for the given object.
    """
    if dim is None:
        return obj.dims
    return (dim,) if isinstance(dim, str) else tuple(dim)


def reduced_coords(da: DataArray, dim: Dims) -> dict[str, Variable]:
    return _reduced(da.coords, concrete_dims(da, dim))


def reduced_attrs(da: DataArray, dim: Dims) -> dict[str, Variable]:
    return _reduced(da.deprecated_attrs, concrete_dims(da, dim))


def reduced_masks(da: DataArray, dim: Dims) -> dict[str, Variable]:
    return _copied(_reduced(da.masks, concrete_dims(da, dim)))


def irreducible_mask(da: DataArray, dim: Dims) -> None | Variable:
    """
    The union of masks that would need to be applied in a reduction op over dim.

    Irreducible means that a reduction operation must apply these masks since they
    depend on the reduction dimensions. Returns None if there is no irreducible mask.
    """
    dims = set(concrete_dims(da, dim))
    irreducible = [mask for mask in da.masks.values() if not dims.isdisjoint(mask.dims)]
    if len(irreducible) == 0:
        return None

    def _transposed_like_data(x):
        return x.transpose([dim for dim in da.dims if dim in x.dims])

    if len(irreducible) == 1:
        return _transposed_like_data(irreducible[0]).copy()
    return _transposed_like_data(reduce(logical_or, irreducible))
