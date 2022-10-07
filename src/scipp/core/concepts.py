# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from functools import reduce
from typing import Callable, Dict, List, Tuple, Union
from .cpp_classes import DataArray, Variable
from ..typing import Dims, VariableLikeType
from .logical import logical_or


def rewrap_output_data(prototype: VariableLikeType, data) -> VariableLikeType:
    if isinstance(prototype, DataArray):
        return DataArray(data=data,
                         coords={c: coord
                                 for c, coord in prototype.coords.items()},
                         attrs={a: attr
                                for a, attr in prototype.attrs.items()},
                         masks={m: mask.copy()
                                for m, mask in prototype.masks.items()})
    else:
        return data


def transform_data(obj: VariableLikeType, func: Callable) -> VariableLikeType:
    if isinstance(obj, Variable):
        return func(obj)
    if isinstance(obj, DataArray):
        return rewrap_output_data(obj, func(obj.data))
    else:
        raise TypeError(f"{func} only supports Variable and DataArray as inputs.")


def concrete_dims(obj: VariableLikeType, dim: Dims) -> Tuple[str]:
    """Convert a dimension specification into a concrete tuple of dimension labels.

    This does *not* validate that the dimension labels are valid for the given object.
    """
    if dim is None:
        return obj.dims
    return (dim, ) if isinstance(dim, str) else tuple(dim)


def _reduced(obj: Dict[str, Variable], dims: List[str]) -> Dict[str, Variable]:
    dims = set(dims)
    return {name: var for name, var in obj.items() if dims.isdisjoint(var.dims)}


def reduced_coords(da: DataArray, dim: Dims) -> Dict[str, Variable]:
    return _reduced(da.coords, concrete_dims(da, dim))


def reduced_attrs(da: DataArray, dim: Dims) -> Dict[str, Variable]:
    return _reduced(da.attrs, concrete_dims(da, dim))


def reduced_masks(da: DataArray, dim: Dims) -> Dict[str, Variable]:
    return {
        name: mask.copy()
        for name, mask in _reduced(da.masks, concrete_dims(da, dim)).items()
    }


def irreducible_mask(da: DataArray, dim: Dims) -> Union[None, Variable]:
    """
    The union of masks that would need to be applied in a reduction op over dim.

    Irreducible means that a reduction operation must apply these masks since they
    depend on the reduction dimensions. Returns None if there is no irreducible mask.
    """
    dims = set(concrete_dims(da, dim))
    irreducible = [mask for mask in da.masks.values() if not dims.isdisjoint(mask.dims)]
    if len(irreducible) == 0:
        return None
    if len(irreducible) == 1:
        return irreducible[0].copy()
    return reduce(logical_or, irreducible)
