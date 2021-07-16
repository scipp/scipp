# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

import inspect
from typing import Union
from . import Variable, DataArray, Dataset


def _add_coord(*, name, obj, tree):
    if name in obj.meta:
        return _produce_coord(obj, name)
    if isinstance(tree[name], str):
        out, dim = _get_coord(tree[name], obj, tree)
    else:
        func = tree[name]
        args = inspect.getfullargspec(func).kwonlyargs
        params = {}
        dims = []
        for arg in args:
            coord, dim = _get_coord(arg, obj, tree)
            params[arg] = coord
            dims += [] if dim is None else [dim]
        dim = dims[0] if len(dims) == 1 else None
        out = func(**params)
    if isinstance(out, Variable):
        out = {name: out}
    for key, coord in out.items():
        obj.coords[key] = coord
    # TODO How can we prevent rename if there are multiple consumers?
    if dim is not None:
        obj.rename_dims({dim: name})


def _consume_coord(obj, name):
    if name in obj.coords:
        obj.attrs[name] = obj.coords[name]
        del obj.coords[name]
    return obj.attrs[name]


def _produce_coord(obj, name):
    if name in obj.attrs:
        obj.coords[name] = obj.attrs[name]
        del obj.attrs[name]
    return obj.coords[name]


def _get_coord(name, obj, tree):
    if name in obj.meta:
        coord = _consume_coord(obj, name)
        dim = name if name in coord.dims else None
        return coord, dim
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
