# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

import inspect
from typing import Union
from . import Variable, DataArray, Dataset


def _add_coord(*, name, obj, tree):
    if isinstance(tree[name], str):
        out = _get_coord(tree[name], obj, tree)
    else:
        func = tree[name]
        args = inspect.getfullargspec(func).kwonlyargs
        out = func(**{arg: _get_coord(arg, obj, tree) for arg in args})
    if isinstance(out, Variable):
        out = {name: out}
    for key, coord in out.items():
        obj.coords[key] = coord


def _consume_coord(obj, name):
    if name in obj.coords:
        obj.attrs[name] = obj.coords[name]
        del obj.coords[name]
    return obj.attrs[name]


def _get_coord(name, obj, tree):
    if name in obj.meta:
        return _consume_coord(obj, name)
    else:
        _add_coord(name=name, obj=obj, tree=tree)
        return _get_coord(name, obj, tree)


def transform_coords(obj: Union[DataArray, Dataset], coords,
                     tree: dict) -> Union[DataArray, Dataset]:
    """
    """
    # Keys in tree may be tuple to define multiple outputs
    simple_tree = {}
    for key in tree:
        for k in [key] if isinstance(key, str) else key:
            simple_tree[k] = tree[key]
    obj = obj.copy(deep=False)
    for name in [coords] if isinstance(coords, str) else coords:
        _add_coord(name=name, obj=obj, tree=simple_tree)
    return obj
