# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

import inspect
from typing import Union, List, Dict, Tuple, Callable
from .core import DataArray, Dataset, bins, VariableError, identical


def _argnames(func):
    spec = inspect.getfullargspec(func)
    if spec.varargs is not None or spec.varkw is not None:
        raise ValueError(
            "Function with variable arguments not allowed in conversion graph.")
    return spec.args + spec.kwonlyargs


def _make_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError:
        raise RuntimeError("Failed to import `graphviz`, please install `graphviz` if "
                           "using `pip`, or `python-graphviz` if using `conda`.")
    return Digraph(*args, **kwargs)


class Graph:
    def __init__(self, graph):
        # Keys in graph may be tuple to define multiple outputs
        self._graph = {}
        for key in graph:
            for k in [key] if isinstance(key, str) else key:
                if k in self._graph:
                    raise ValueError("Duplicate output name define in conversion graph")
                self._graph[k] = graph[key]

    def __getitem__(self, name):
        return self._graph[name]

    def __contains__(self, name):
        return name in self._graph

    def show(self, size=None, simplified=False):
        dot = _make_digraph(strict=True)
        dot.attr('node', shape='box', height='0.1')
        dot.attr(size=size)
        for output, producer in self._graph.items():
            if isinstance(producer, str):  # rename
                dot.edge(producer, output)
            else:
                if not simplified:
                    name = f'{producer.__name__}(...)'
                    dot.node(name, shape='ellipse', style='filled', color='lightgrey')
                    dot.edge(name, output)
                else:
                    name = output
                argnames = _argnames(producer)
                for arg in argnames:
                    dot.edge(arg, name)
        return dot


def _move_between_member_dicts(obj, name, src_name, dst_name):
    src = getattr(obj, src_name)
    dst = getattr(obj, dst_name)
    if name in src:
        dst[name] = src.pop(name)
    return dst.get(name, None)


def _move_between_coord_and_attr(obj, name, src_name, dst_name):
    return (_move_between_member_dicts(obj, name, src_name, dst_name),
            _move_between_member_dicts(obj.bins, name, src_name, dst_name)
            if obj.bins is not None else None)


def _consume_coord(obj, name):
    return _move_between_coord_and_attr(obj, name, 'coords', 'attrs')


def _produce_coord(obj, name):
    return _move_between_coord_and_attr(obj, name, 'attrs', 'coords')


def _store_event_coord(obj, name, coord):
    try:
        obj.bins.coords[name] = coord
    except VariableError:  # Thrown on mismatching bin indices, e.g. slice
        obj.data = obj.data.copy()
        obj.bins.coords[name] = coord
    if name in obj.bins.attrs:
        del obj.bins.attrs[name]


def _store_coord(obj, name, coord):
    dense_coord, event_coord = coord
    if dense_coord is not None:
        obj.coords[name] = dense_coord
    if name in obj.attrs:
        # If name is both an input and output to a function,
        # the input handling made it an attr, but since it is
        # an output, we want to store it as a coord (and only as a coord).
        del obj.attrs[name]
    if event_coord is not None:
        _store_event_coord(obj, name, event_coord)


class CoordTransform:
    Graph = Dict[Union[str, Tuple[str, ...]], Union[str, Callable]]

    def __init__(self, obj, *, graph, outputs):
        self.obj = obj.copy(deep=False)
        # TODO We manually shallow-copy the buffer, until we have a better
        # solution for how shallow copies also shallow-copy event buffers.
        if self.obj.bins is not None:
            self.obj.data = bins(**self.obj.bins.constituents)
        self._original = obj
        self._events_copied = False
        self._rename = {}
        self._memo = []  # names of product for cycle detection
        self._aliases = []  # names that alias other names
        self._consumed = []  # names that have been consumed
        self._outputs = outputs  # names of outputs
        self._graph = graph

    def _add_coord(self, *, name):
        if self._exists(name):
            return _produce_coord(self.obj, name)
        if isinstance(self._graph[name], str):
            self._aliases.append(name)
            out = {name: self._get_coord(self._graph[name])}
            dim = (self._graph[name], )
        else:
            func = self._graph[name]
            argnames = _argnames(func)
            args = {arg: self._get_coord(arg) for arg in argnames}
            have_all_dense_inputs = all([v[0] is not None for v in args.values()])
            if have_all_dense_inputs:
                dense_args = {k: v[0] for k, v in args.items()}
                out = func(**dense_args)
                if not isinstance(out, dict):
                    out = {name: out}
            else:
                out = {}
            have_event_inputs = any([v[1] is not None for v in args.values()])
            if have_event_inputs:
                event_args = {
                    k: v[0] if v[1] is None else v[1]
                    for k, v in args.items()
                }
                out_bins = func(**event_args)
                if not isinstance(out_bins, dict):
                    out_bins = {name: out_bins}
                # Dense outputs may be produced as side effects of processing event
                # coords.
                for name in list(out_bins.keys()):
                    if out_bins[name].bins is None:
                        coord = out_bins.pop(name)
                        if name in out:
                            assert identical(out[name], coord)
                        else:
                            out[name] = coord
            else:
                out_bins = {}
            out = {
                k: (out.get(k, None), out_bins.get(k, None))
                for k in list(out.keys()) + list(out_bins.keys())
            }
            dim = tuple(argnames)
        self._rename.setdefault(dim, []).extend(out.keys())
        for key, coord in out.items():
            _store_coord(self.obj, key, coord)

    def _exists(self, name):
        in_events = self.obj.bins is not None and name in self.obj.bins.meta
        return name in self.obj.meta or in_events

    def _get_existing(self, name):
        events = None if self.obj.bins is None else self.obj.bins.meta[name]
        return self.obj.meta.get(name, None), events

    def _get_coord(self, name):
        if self._exists(name):
            self._consumed.append(name)
            if name in self._outputs:
                return self._get_existing(name)
            else:
                return _consume_coord(self.obj, name)
        else:
            if name in self._memo:
                raise ValueError("Cycle detected in conversion graph.")
            self._memo.append(name)
            self._add_coord(name=name)
            return self._get_coord(name)

    def _del_attr(self, name):
        self.obj.attrs.pop(name, None)
        if self.obj.bins is not None:
            self.obj.bins.attrs.pop(name, None)

    def _del_coord(self, name):
        self.obj.coords.pop(name, None)
        if self.obj.bins is not None:
            self.obj.bins.coords.pop(name, None)

    def finalize(self, *, include_aliases, rename_dims, keep_intermediate, keep_inputs):
        for name in self._outputs:
            self._add_coord(name=name)
        if not include_aliases:
            for name in self._aliases:
                self._del_attr(name)
        for name in self._consumed:
            if name not in self.obj.attrs:
                continue
            if name in self._original.meta:
                if not keep_inputs:
                    self._del_attr(name)
            elif not keep_intermediate:
                self._del_attr(name)
        unconsumed = set(self.obj.coords) - set(self._original.meta) - set(
            self._outputs)
        for name in unconsumed:
            if name not in self._graph:
                self._del_coord(name)
        if rename_dims:
            blacklist = _get_splitting_nodes(self._rename)
            for key, val in self._rename.items():
                found = [k for k in key if k in self.obj.dims]
                # rename if exactly one input is dimension-coord
                if len(val) == 1 and len(found) == 1 and found[0] not in blacklist:
                    self.obj = self.obj.rename_dims({found[0]: val[0]})
        return self.obj


def _get_splitting_nodes(graph):
    nodes = {}
    for key in graph:
        for start in key:
            nodes[start] = nodes.get(start, 0) + 1
    return [node for node in nodes if nodes[node] > 1]


def _transform_data_array(obj: DataArray, coords, graph: dict, *, kwargs) -> DataArray:
    transform = CoordTransform(obj,
                               graph=Graph(graph),
                               outputs=[coords] if isinstance(coords, str) else coords)
    return transform.finalize(**kwargs)


def _transform_dataset(obj: Dataset, coords, graph: dict, *, kwargs) -> Dataset:
    # Note the inefficiency here in datasets with multiple items: Coord
    # transform is repeated for every item rather than sharing what is
    # possible. Implementing this would be tricky and likely error-prone,
    # since different items may have different attributes. Unless we have
    # clear performance requirements we therefore go with the safe and
    # simple solution
    return Dataset(
        data={
            name: _transform_data_array(
                obj[name], coords=coords, graph=graph, kwargs=kwargs)
            for name in obj
        })


def transform_coords(x: Union[DataArray, Dataset],
                     coords: Union[str, List[str]],
                     graph: CoordTransform.Graph,
                     *,
                     rename_dims=True,
                     include_aliases=True,
                     keep_intermediate=True,
                     keep_inputs=True) -> Union[DataArray, Dataset]:
    """Compute new coords based on transformation of input coords.

    :param x: Input object with coords.
    :param coords: Name or list of names of desired output coords.
    :param graph: A graph defining how new coords can be computed from existing
                  coords. This may be done in multiple steps.
                  The graph is given by a `dict` where:

                  - Dict keys are `str` or `tuple` of `str`, defining the
                    names of outputs generated by a dict value.
                  - Dict values are `str` or a callable (function). If `str`,
                    this is a synonym for renaming a coord. If a callable,
                    it must either return a single variable or a dict of
                    variables.
    :param rename_dims: Rename dimensions if products of dimension coord are
                        fully consumed and consumer consumes exectly one
                        dimension coordinate. Default is True.
    :param include_aliases: If True, aliases for coords defined in graph are
                            included in the output. Default is False.
    :param keep_intermediate: Keep attributes created as intermediate results.
                              Default is True.
    :param keep_inputs: Keep consumed input coordinates or attributes.
                        Default is True.
    :return: New object with desired coords. Existing data and meta-data is
             shallow-copied.
    """
    kwargs = {
        'rename_dims': rename_dims,
        'include_aliases': include_aliases,
        'keep_intermediate': keep_intermediate,
        'keep_inputs': keep_inputs
    }
    if isinstance(x, DataArray):
        return _transform_data_array(x, coords=coords, graph=graph, kwargs=kwargs)
    else:
        return _transform_dataset(x, coords=coords, graph=graph, kwargs=kwargs)


def show_graph(graph, size: str = None, simplified: bool = False):
    """
    Show graphical representation of a graph as required by
    :py:func:`transform_coords`

    Requires `python-graphviz` package.

    :param size: Size forwarded to graphviz, must be a string, "width,height"
                 or "size". In the latter case, the same value is used for
                 both width and height.
    :param simplified: If ``True``, do not show the conversion functions,
                       only the potential input and output coordinates.
    """
    return Graph(graph).show(size=size, simplified=simplified)
