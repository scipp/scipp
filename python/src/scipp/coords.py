# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

import inspect
from typing import Union
from . import Variable, DataArray, Dataset


def _add_coord(*, name, obj, tree):
    if name in obj.meta:
        # TODO This must return a dim, but cannot find the correct one
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
    return dim
    # if dim is not None:
    #     obj.rename_dims({dim: name})


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


# y -> y_square -> abc
# _add_coord(name='abc')
#     _get_coord(name='y_square')
#         _add_coord(name='y_square')
#             _get_coord(name='y')
#                 return dim='y'
#             return dim='y'
#         _get_coord(name='y_square', dim_name='y')
#             dims=['y']
#             return dim='y'
#         return dim='y'
#
#


def _get_coord(name, obj, tree, dim_name=None):
    if name in obj.meta:
        coord = _consume_coord(obj, name)
        dim_name = name if dim_name is None else dim_name
        dim = dim_name if dim_name in coord.dims else None
        return coord, dim
    else:
        dim = _add_coord(name=name, obj=obj, tree=tree)
        return _get_coord(name, obj, tree, dim_name=dim)


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
        dim = _add_coord(name=name, obj=obj, tree=simple_tree)
        print(dim)
        rename.setdefault(dim, []).append(name)
        print(rename)
    for dim in rename:
        if len(rename[dim]) == 1:
            obj.rename_dims({dim: rename[dim][0]})
    return obj
