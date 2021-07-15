# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Hezbrock

import inspect
from typing import Union
from . import DataArray, Dataset


def _get_coord(name, obj, tree):
    if name in obj.coords:
        obj.attrs[name] = obj.coords[name]
        del obj.coords[name]
        return obj.attrs[name]
    elif isinstance(tree[name], str):
        return _get_coord(tree[name], obj, tree)
    else:
        func = tree[name]
        args = inspect.getfullargspec(func).kwonlyargs
        return func(**{arg: _get_coord(arg, obj, tree) for arg in args})


def transform_coords(obj: Union[DataArray, Dataset], converter,
                     tree: dict) -> Union[DataArray, Dataset]:
    obj = obj.copy(deep=False)
    args = inspect.getfullargspec(converter).kwonlyargs
    coords = converter(**{arg: _get_coord(arg, obj, tree) for arg in args})
    for key, coord in coords.items():
        obj.coords[key] = coord
    return obj
