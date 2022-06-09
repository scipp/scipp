# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from functools import wraps
from typing import Callable, Dict, Union

from .._scipp import core as _cpp
from .variable import Variable, linspace, arange
from .bins import bin, histogram


def _get_coord(x, name):
    event_coord = x.bins.meta.get(name) if x.bins is not None else None
    return x.meta.get(name, event_coord)


def _upper_bound(x):
    import numpy as np
    bound = x.max()
    bound.value = np.nextafter(bound.value, np.inf)
    return bound


def _parse_coords_arg(x, name, arg):
    coord = _get_coord(x, name)
    if isinstance(arg, int):
        coord = linspace(name, coord.min(), _upper_bound(coord), num=arg + 1)
    elif isinstance(arg, Variable):
        if arg.ndim == 0:
            start = coord.min()
            step = arg.to(dtype=start.dtype, unit=start.unit)
            stop = _upper_bound(coord) + step
            coord = arange(name, start, stop, step=step)
        else:
            coord = arg
    return coord


def make_edges_func_1d(name: str, func: Callable) -> Callable:

    @wraps(func)
    def function(x: Union[_cpp.DataArray, _cpp.Dataset],
                 edges: Dict[str, Union[int, Variable]] = None,
                 /,
                 **kwargs) -> Union[_cpp.DataArray, _cpp.Dataset]:
        if edges is not None:
            kwargs = dict(**edges, **kwargs)
        if len(kwargs) != 1:
            raise ValueError("Currently only 1-D histogramming is supported.")
        name, arg = next(iter(kwargs.items()))
        return func(x, bins=_parse_coords_arg(x, name, arg))

    return function


def hist(x: Union[_cpp.DataArray, _cpp.Dataset],
         arg_dict: Dict[str, Union[int, Variable]] = None,
         /,
         **kwargs) -> Union[_cpp.DataArray, _cpp.Dataset]:
    if arg_dict is not None:
        kwargs = dict(**arg_dict, **kwargs)
    edges = [_parse_coords_arg(x, name, arg) for name, arg in kwargs.items()]
    if len(edges) == 0:
        return x.bins.sum()
    if len(edges) == 1:
        return histogram(x, bins=edges[0])
    else:
        return histogram(bin(x, edges=[edges[:-1]]), bins=edges[-1])
