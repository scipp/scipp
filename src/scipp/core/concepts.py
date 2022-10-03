# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Callable
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
    # TODO Dataset
