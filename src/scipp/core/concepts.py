# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from collections.abc import Callable, Iterable, Mapping
from functools import reduce
from typing import TypeVar

from ..typing import Dims, VariableLikeType
from .cpp_classes import DataArray, DimensionError, Variable
from .logical import logical_or

_VarOrDa = TypeVar('_VarOrDa', Variable, DataArray)


def _copied(obj: Mapping[str, Variable]) -> dict[str, Variable]:
    return {name: var.copy() for name, var in obj.items()}


def _reduced(obj: Mapping[str, Variable], dims: Iterable[str]) -> dict[str, Variable]:
    ref_dims = set(dims)
    return {name: var for name, var in obj.items() if ref_dims.isdisjoint(var.dims)}


def rewrap_output_data(prototype: _VarOrDa, data: Variable) -> _VarOrDa:
    if isinstance(prototype, DataArray):
        return DataArray(
            data=data,
            coords=prototype.coords,
            masks=_copied(prototype.masks),
        )
    else:
        return data


def rewrap_reduced_data(prototype: DataArray, data: Variable, dim: Dims) -> DataArray:
    return DataArray(
        data,
        coords=reduced_coords(prototype, dim),
        masks=reduced_masks(prototype, dim),
    )


def transform_data(
    obj: VariableLikeType, func: Callable[[Variable], Variable]
) -> VariableLikeType:
    if isinstance(obj, Variable):
        return func(obj)
    if isinstance(obj, DataArray):
        return rewrap_output_data(obj, func(obj.data))
    else:
        raise TypeError(f"{func} only supports Variable and DataArray as inputs.")


def concrete_dims(obj: VariableLikeType, dim: Dims) -> tuple[str, ...]:
    """Convert a dimension specification into a concrete tuple of dimension labels.

    This does *not* validate that the dimension labels are valid for the given object.
    """
    if dim is None:
        if None in obj.dims:
            raise DimensionError(
                f'Got data group with unequal dimension lengths: dim={obj.dims}'
            )
        return obj.dims
    return (dim,) if isinstance(dim, str) else tuple(dim)


def reduced_coords(da: DataArray, dim: Dims) -> dict[str, Variable]:
    return _reduced(da.coords, concrete_dims(da, dim))


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

    def _transposed_like_data(x: Variable) -> Variable:
        return x.transpose([d for d in da.dims if d in x.dims])

    if len(irreducible) == 1:
        return _transposed_like_data(irreducible[0]).copy()
    return _transposed_like_data(reduce(logical_or, irreducible))
