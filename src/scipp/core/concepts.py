# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Callable, Dict, Sequence, Union
from .cpp_classes import DataArray, Variable
from ..typing import VariableLikeType


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


Dims = Union[str, Sequence[str]]


def _reduced(obj: Dict[str, Variable], dim: Dims) -> Dict[str, Variable]:
    dims = set([dim] if isinstance(dim, str) else dim)
    return {name: var for name, var in obj.items() if dims.isdisjoint(var.dims)}


def reduced_coords(da: DataArray, dim: Dims) -> Dict[str, Variable]:
    return _reduced(da.coords, dim)


def reduced_attrs(da: DataArray, dim: Dims) -> Dict[str, Variable]:
    return _reduced(da.attrs, dim)


def reduced_masks(da: DataArray, dim: Dims) -> Dict[str, Variable]:
    return {name: mask.copy() for name, mask in _reduced(da.masks, dim).items()}
