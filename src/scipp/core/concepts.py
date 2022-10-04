# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Callable, Dict
from .cpp_classes import DataArray, Variable, Dataset
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


def _copy_dict_for_overwrite(mapping: Dict[str, Variable]) -> Dict[str, Variable]:
    return {name: copy_for_overwrite(var) for name, var in mapping.items()}


def copy_for_overwrite(obj: VariableLikeType) -> VariableLikeType:
    """
    Copy a Scipp object for overwriting.

    Unlike :py:func:`scipp.empty_like` this does not preserve (and share) coord,
    mask, and attr values. Instead, those values are not initialized, just like the
    data values.
    """
    from .like import empty_like
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
