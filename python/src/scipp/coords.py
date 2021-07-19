# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

import inspect
from typing import Union, List, Dict, Tuple, Callable
from . import Variable, DataArray, Dataset, bins, VariableError


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


class CoordTransform:
    Graph = Dict[Union[str, Tuple[str, ...]], Union[str, Callable]]

    def __init__(self, obj):
        self.obj = obj
        self._events_copied = False

    def _add_event_coord(self, key, coord):
        try:
            self.obj.bins.coords[key] = coord
        except VariableError:  # Thrown on mismatching bin indices, e.g. slice
            self.obj.data = self.obj.data.copy()
            self.obj.bins.coords[key] = coord

    def _add_event_coords(self, coords):
        for key, coord in coords.items():
            # Non-binned coord should be duplicate of dense handling above,
            # if present => ignored.
            if coord.bins is not None:
                self._add_event_coord(key, coord)

    def _add_coord(self, *, name, graph, rename):
        if name in self.obj.meta:
            return _produce_coord(self.obj, name)
        if isinstance(graph[name], str):
            out = self._get_coord(graph[name], graph, rename)
            if self.obj.bins is not None:
                # Calls to _get_coord for dense coord handling take care of
                # recursion and add also event coords, here and below we thus
                # simply consume the event coord.
                if graph[name] in self.obj.meta:
                    out_bins = _consume_coord(self.obj.bins, graph[name])
            dim = (graph[name], )
        else:
            func = graph[name]
            argnames = inspect.getfullargspec(func).kwonlyargs
            args = {
                arg: self._get_coord(arg, graph, rename)
                for arg in argnames
            }
            out = func(**args)
            if self.obj.bins is not None:
                args.update({
                    arg: _consume_coord(self.obj.bins, arg)
                    for arg in argnames if arg in self.obj.bins.meta
                })
                out_bins = func(**args)
            dim = tuple(argnames)
        if isinstance(out, Variable):
            out = {name: out}
        rename.setdefault(dim, []).extend(out.keys())
        for key, coord in out.items():
            self.obj.coords[key] = coord
        if self.obj.bins is not None:
            if isinstance(out_bins, Variable):
                out_bins = {name: out_bins}
            self._add_event_coords(out_bins)

    def _get_coord(self, name, graph, rename):
        if name in self.obj.meta:
            return _consume_coord(self.obj, name)
        else:
            self._add_coord(name=name, graph=graph, rename=rename)
            return self._get_coord(name, graph, rename=rename)


def _get_splitting_nodes(graph):
    nodes = {}
    for key in graph:
        for start in key:
            nodes[start] = nodes.get(start, 0) + 1
    return [node for node in nodes if nodes[node] > 1]


def _transform_data_array(obj: DataArray, coords, graph: dict) -> DataArray:
    # Keys in graph may be tuple to define multiple outputs
    simple_graph = {}
    for key in graph:
        for k in [key] if isinstance(key, str) else key:
            simple_graph[k] = graph[key]
    obj = obj.copy(deep=False)
    # TODO We manually shallow-copy the buffer, until we have a better
    # solution for how shallow copies also shallow-copy event buffers.
    if obj.bins is not None:
        obj.data = bins(**obj.bins.constituents)
    rename = {}
    transform = CoordTransform(obj)
    for name in [coords] if isinstance(coords, str) else coords:
        transform._add_coord(name=name, graph=simple_graph, rename=rename)
    blacklist = _get_splitting_nodes(rename)
    for key, val in rename.items():
        found = [k for k in key if k in obj.dims]
        # rename if exactly one input is dimension-coord
        if len(val) == 1 and len(found) == 1 and found[0] not in blacklist:
            obj = obj.rename_dims({found[0]: val[0]})
    return obj


def _transform_dataset(obj: Dataset, coords, graph: dict) -> Dataset:
    # Note the inefficiency here in datasets with multiple items: Coord
    # transform is repeated for every item rather than sharing what is
    # possible. Implementing this would be tricky and likely error-prone,
    # since different items may have different attributes. Unless we have
    # clear performance requirements we therefore go with the safe and
    # simple solution
    return Dataset(
        data={
            name: _transform_data_array(obj[name], coords=coords, graph=graph)
            for name in obj
        })


def transform_coords(x: Union[DataArray, Dataset], coords: Union[str,
                                                                 List[str]],
                     graph: CoordTransform.Graph) -> Union[DataArray, Dataset]:
    """Compute new coords based on transformation of input coords.

    :param x: Input object with coords.
    :param coords: Name or list of names of desired output coords.
    :param graph: A graph defining how new coords can be computed from existing
                  coords. This may be done in multiple steps.
    :return: New object with desired coords. Existing data and meta-data is
             shallow-copied.
    """
    if isinstance(x, DataArray):
        return _transform_data_array(x, coords=coords, graph=graph)
    else:
        return _transform_dataset(x, coords=coords, graph=graph)
