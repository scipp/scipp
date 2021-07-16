# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

import inspect
from typing import Union
from . import Variable, DataArray, Dataset


def _add_coord(*, name, obj, tree, rename):
    if name in obj.meta:
        return _produce_coord(obj, name)
    if isinstance(tree[name], str):
        out = _get_coord(tree[name], obj, tree, rename)
        dim = (tree[name], )
    else:
        func = tree[name]
        args = inspect.getfullargspec(func).kwonlyargs
        out = func(**{arg: _get_coord(arg, obj, tree, rename) for arg in args})
        dim = tuple(args)
    if isinstance(out, Variable):
        out = {name: out}
    rename.setdefault(dim, []).extend(out.keys())
    for key, coord in out.items():
        obj.coords[key] = coord


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


def _get_coord(name, obj, tree, rename):
    if name in obj.meta:
        return _consume_coord(obj, name)
    else:
        _add_coord(name=name, obj=obj, tree=tree, rename=rename)
        return _get_coord(name, obj, tree, rename=rename)


def _get_splitting_nodes(graph):
    nodes = {}
    for key in graph:
        for start in key:
            nodes[start] = nodes.get(start, 0) + 1
    return [node for node in nodes if nodes[node] > 1]


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
    rename = {}
    for name in [coords] if isinstance(coords, str) else coords:
        _add_coord(name=name, obj=obj, tree=simple_tree, rename=rename)
    blacklist = _get_splitting_nodes(rename)
    for key in rename:
        if len(rename[key]) == 1:
            found = [k for k in key if k in obj.dims]
            # rename if exactly one input is dimension-coord
            if len(found) == 1 and found[0] not in blacklist:
                obj.rename_dims({found[0]: rename[key][0]})
    return obj
