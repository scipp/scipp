# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

from abc import ABC, abstractmethod
from graphlib import TopologicalSorter
import inspect
from typing import Callable, Dict, Iterable, List, Mapping,\
    Optional, Tuple, Union

from .core import CoordError, DataArray, Dataset, VariableError,\
    Variable, bins, identical
from .logging import get_logger

_OptionalCoordTuple = Tuple[Optional[Variable], Optional[Variable]]
GraphDict = Dict[Union[str, Tuple[str, ...]], Union[str, Callable]]


def _argnames(func) -> Tuple[str]:
    spec = inspect.getfullargspec(func)
    if spec.varargs is not None or spec.varkw is not None:
        raise ValueError(
            "Function with variable arguments not allowed in conversion graph.")
    return tuple(spec.args + spec.kwonlyargs)


def _make_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError:
        raise RuntimeError("Failed to import `graphviz`, please install `graphviz` if "
                           "using `pip`, or `python-graphviz` if using `conda`.")
    return Digraph(*args, **kwargs)


class _Rule(ABC):
    def __init__(self, out_names: Union[str, Tuple[str]]):
        self.out_names = (out_names, ) if isinstance(out_names, str) else out_names

    @abstractmethod
    def __call__(self, coords: Dict[str, Variable]) -> Dict[str, Variable]:
        """Evaluate the kernel."""

    @abstractmethod
    def dependencies(self) -> Iterable[str]:
        """Return names of coords that this kernel needs as inputs."""

    @staticmethod
    def _get_coord(name: str, coords: Mapping[str, Variable]) -> Variable:
        try:
            return coords[name]
        except KeyError:
            raise CoordError(f'Internal Error: Coordinate {name} does is not in the '
                             'input or has not (yet) been computed')

    def _format_out_names(self):
        return str(self.out_names) if len(self.out_names) > 1 else self.out_names[0]


class _FetchRule(_Rule):
    def __init__(self, out_names: Union[str, Tuple[str, ...]],
                 sources: Mapping[str, Variable]):
        super().__init__(out_names)
        self._sources = sources

    def __call__(self, _) -> Dict[str, Variable]:
        return {
            out_name: self._get_coord(out_name, self._sources)
            for out_name in self.out_names
        }

    def dependencies(self) -> Iterable[str]:
        return ()

    def __str__(self):
        return f'Input   {self._format_out_names()}'


class _RenameRule(_Rule):
    def __init__(self, out_names: Union[str, Tuple[str, ...]], in_name: str):
        super().__init__(out_names)
        self._in_name = in_name

    def __call__(self, coords: Mapping[str, Variable]) -> Dict[str, Variable]:
        return {
            out_name: self._get_coord(self._in_name, coords)
            for out_name in self.out_names
        }

    def dependencies(self) -> Iterable[str]:
        return tuple((self._in_name, ))

    def __str__(self):
        return f'Rename  {self._format_out_names()} = {self._in_name}'


class _ComputeRule(_Rule):
    def __init__(self, out_names: Union[str, Tuple[str, ...]], func: Callable):
        super().__init__(out_names)
        self._func = func
        self._arg_names = _argnames(func)

    def __call__(self, coords: Mapping[str, Variable]) -> Dict[str, Variable]:
        res = self._func(
            **{name: self._get_coord(name, coords)
               for name in self._arg_names})
        if not isinstance(res, dict):
            if len(self.out_names) != 1:
                raise TypeError('Function returned a single output but '
                                f'{len(self.out_names)} were expected.')
            res = {self.out_names[0]: res}
        return res

    def dependencies(self) -> Iterable[str]:
        return self._arg_names

    def __str__(self):
        return f'Compute {self._format_out_names()} = {self._func.__name__}' \
               f'({", ".join(self._arg_names)})'


def _make_rule(products, producer) -> _Rule:
    if isinstance(producer, str):
        return _RenameRule(products, producer)
    return _ComputeRule(products, producer)


def _non_duplicate_rules(rules: Mapping[str, _Rule]) -> List[_Rule]:
    already_used = set()
    result = []
    for rule in filter(lambda r: r not in already_used,
                       map(lambda n: rules[n], _sort_topologically(rules))):
        already_used.add(rule)
        result.append(rule)
    return result


def _convert_to_rule_graph(graph: GraphDict) -> Dict[str, _Rule]:
    rule_graph = {}
    for products, producer in graph.items():
        rule = _make_rule(products, producer)
        for product in (products, ) if isinstance(products, str) else products:
            if product in rule_graph:
                raise ValueError(
                    f'Duplicate output name defined in conversion graph: {product}')
            rule_graph[product] = rule
    return rule_graph


def _sort_topologically(graph: Mapping[str, _Rule]) -> Iterable[str]:
    yield from TopologicalSorter(
        {out: rule.dependencies()
         for out, rule in graph.items()}).static_order()


class Graph:
    def __init__(self, graph):
        # Keys in graph may be tuple to define multiple outputs
        self._graph = {}
        for key in graph:
            for k in [key] if isinstance(key, str) else key:
                if k in self._graph:
                    raise ValueError("Duplicate output name define in conversion graph")
                self._graph[k] = graph[key]
        self._rule_graph = _convert_to_rule_graph(graph)

    def subgraph(self, data: DataArray, targets: Tuple[str, ...]) -> Dict[str, _Rule]:
        subgraph = {}
        pending = list(targets)
        while pending:
            out_name = pending.pop()
            if out_name in subgraph:
                continue
            rule = self._rule_for(out_name, data)
            subgraph[out_name] = rule
            pending.extend(rule.dependencies())
        return subgraph

    def _rule_for(self, out_name: str, data: DataArray) -> _Rule:
        if out_name in data.meta:
            # TODO bins
            return _FetchRule(out_name, data.meta)
        try:
            return self._rule_graph[out_name]
        except KeyError:
            raise CoordError(
                f"Coordinate '{out_name}' does not exist in the input data "
                "and no rule has been provided to compute it.") from None

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
                for arg in _argnames(producer):
                    dot.edge(arg, name)
        return dot


def _log_plan(rules: List[_Rule]) -> None:
    get_logger().info('Transforming coords\n%s',
                      '\n'.join(f'  {rule}' for rule in rules))


def _store_results(x: DataArray, coords: Dict[str, Variable],
                   targets: Tuple[str, ...]) -> DataArray:
    x = x.copy(deep=False)
    for name, coord in coords.items():
        if name in targets:
            x.coords[name] = coord
            if name in x.attrs:
                del x.attrs[name]
        else:
            x.attrs[name] = coord
            if name in x.coords:
                del x.coords[name]
    return x


def _renamable_dims(x: DataArray, rules: List[_Rule]) -> Dict[str, List[str]]:
    res = {}
    for rule in filter(lambda r: isinstance(r, _FetchRule), rules):
        for name in rule.out_names:
            if name in x.dims:
                res[name] = []
    return res


def _rename_dims(x, original, rename):
    ren = {}
    for dim in original.dims:
        to = dim
        while True:
            try:
                if len(rename[to]) == 1:
                    to = rename[to][0]
                else:
                    break
            except KeyError:
                break
        if dim != to:
            ren[dim] = to
    return x.rename_dims(ren)


def _transform_data_array(x: DataArray, coords: Union[str, List[str], Tuple[str, ...]],
                          graph: GraphDict, options) -> DataArray:
    targets = tuple(coords)
    rules = _non_duplicate_rules(Graph(graph).subgraph(x, targets))
    _log_plan(rules)
    rename_dims = _renamable_dims(x, rules)
    working_coords = {}
    for rule in rules:
        for name, coord in rule(working_coords).items():
            if name in working_coords:
                raise ValueError(f"Coordinate '{name}' was produced multiple times.")
            working_coords[name] = coord
            ren = list(filter(lambda xx: xx in rename_dims, rule.dependencies()))
            if len(ren) == 1:
                rename_dims[ren[0]].append(name)
                rename_dims[name] = []

    res = _store_results(x, working_coords, targets)
    return _rename_dims(res, x, rename_dims)


def _move_between_member_dicts(obj, name: str, src_name: str,
                               dst_name: str) -> Optional[Variable]:
    src = getattr(obj, src_name)
    dst = getattr(obj, dst_name)
    if name in src:
        dst[name] = src.pop(name)
    return dst.get(name, None)


def _move_between_coord_and_attr(obj, name: str, src_name: str,
                                 dst_name: str) -> _OptionalCoordTuple:
    return (_move_between_member_dicts(obj, name, src_name, dst_name),
            _move_between_member_dicts(obj.bins, name, src_name, dst_name)
            if obj.bins is not None else None)


def _consume_coord(obj, name: str) -> _OptionalCoordTuple:
    return _move_between_coord_and_attr(obj, name, 'coords', 'attrs')


def _produce_coord(obj, name: str) -> _OptionalCoordTuple:
    return _move_between_coord_and_attr(obj, name, 'attrs', 'coords')


def _store_event_coord(obj, name: str, coord: Variable) -> None:
    try:
        obj.bins.coords[name] = coord
    except VariableError:  # Thrown on mismatching bin indices, e.g. slice
        obj.data = obj.data.copy()
        obj.bins.coords[name] = coord
    if name in obj.bins.attrs:
        del obj.bins.attrs[name]


def _store_coord(obj, name: str, coord: _OptionalCoordTuple) -> None:
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


def _call_function(func: Callable[..., Union[Variable, Dict[str, Variable]]],
                   args: Dict[str, Variable], out_name: str) -> Dict[str, Variable]:
    out = func(**args)
    if not isinstance(out, dict):
        return {out_name: out}
    for name in out:
        get_logger().info('  %s = %s(%s)', name, func.__name__, ', '.join(args.keys()))
    return out


class CoordTransform:
    def __init__(self, obj: DataArray, *, graph: Graph, outputs: Tuple[str]):
        self.obj = obj.copy(deep=False)
        # TODO We manually shallow-copy the buffer, until we have a better
        # solution for how shallow copies also shallow-copy event buffers.
        if self.obj.bins is not None:
            self.obj.data = bins(**self.obj.bins.constituents)
        self._original = obj
        self._events_copied = False
        self._rename: Dict[Tuple[str], List[str]] = {}
        self._memo: List[str] = []  # names of product for cycle detection
        self._aliases: List[str] = []  # names that alias other names
        self._consumed: List[str] = []  # names that have been consumed
        self._outputs: Tuple[str] = outputs  # names of outputs
        self._graph = graph

    def _add_coord(self, *, name: str):
        if self._exists(name):
            return _produce_coord(self.obj, name)
        if isinstance(self._graph[name], str):
            out, dim = self._rename_coord(name)
        else:
            out, dim = self._compute_coord(name)
        self._rename.setdefault(dim, []).extend(out.keys())
        for key, coord in out.items():
            _store_coord(self.obj, key, coord)

    def _rename_coord(self,
                      name: str) -> Tuple[Dict[str, _OptionalCoordTuple], Tuple[str]]:
        self._aliases.append(name)
        out = {name: self._get_coord(self._graph[name])}
        dim = (self._graph[name], )
        return out, dim

    def _compute_coord(self,
                       name: str) -> Tuple[Dict[str, _OptionalCoordTuple], Tuple[str]]:
        func = self._graph[name]
        argnames = _argnames(func)
        args = {arg: self._get_coord(arg) for arg in argnames}
        have_all_dense_inputs = all([v[0] is not None for v in args.values()])
        if have_all_dense_inputs:
            out = _call_function(func, {k: v[0] for k, v in args.items()}, name)
        else:
            out = {}
        have_event_inputs = any([v[1] is not None for v in args.values()])
        if have_event_inputs:
            event_args = {k: v[0] if v[1] is None else v[1] for k, v in args.items()}
            out_bins = _call_function(func, event_args, name)
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
        return out, dim

    def _exists(self, name: str):
        in_events = self.obj.bins is not None and name in self.obj.bins.meta
        return name in self.obj.meta or in_events

    def _get_existing(self, name: str):
        events = None if self.obj.bins is None else self.obj.bins.meta[name]
        return self.obj.meta.get(name, None), events

    def _get_coord(self, name: str):
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

    def _del_attr(self, name: str):
        self.obj.attrs.pop(name, None)
        if self.obj.bins is not None:
            self.obj.bins.attrs.pop(name, None)

    def _del_coord(self, name: str):
        self.obj.coords.pop(name, None)
        if self.obj.bins is not None:
            self.obj.bins.coords.pop(name, None)

    def finalize(self, *, include_aliases: bool, rename_dims: bool,
                 keep_intermediate: bool, keep_inputs: bool):
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


def _get_splitting_nodes(graph: Dict[Tuple[str], List[str]]) -> List[str]:
    nodes = {}
    for key in graph:
        for start in key:
            nodes[start] = nodes.get(start, 0) + 1
    return [node for node in nodes if nodes[node] > 1]

#
# def _transform_data_array(obj: DataArray, coords , graph: GraphDict, *,
#                           options) -> DataArray:
#     transform = CoordTransform(
#         obj,
#         graph=Graph(graph),
#         outputs=(coords, ) if isinstance(coords, str) else tuple(coords))
#     return transform.finalize(**options)


def _transform_dataset(obj: Dataset, coords: Union[str, List[str], Tuple[str, ...]],
                       graph: GraphDict, *, options) -> Dataset:
    # Note the inefficiency here in datasets with multiple items: Coord
    # transform is repeated for every item rather than sharing what is
    # possible. Implementing this would be tricky and likely error-prone,
    # since different items may have different attributes. Unless we have
    # clear performance requirements we therefore go with the safe and
    # simple solution
    return Dataset(
        data={
            name: _transform_data_array(
                obj[name], coords=coords, graph=graph, options=options)
            for name in obj
        })


def transform_coords(x: Union[DataArray, Dataset],
                     coords: Union[str, List[str], Tuple[str, ...]],
                     graph: GraphDict,
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
                        fully consumed and consumer consumes exactly one
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
    options = {
        'rename_dims': rename_dims,
        'include_aliases': include_aliases,
        'keep_intermediate': keep_intermediate,
        'keep_inputs': keep_inputs
    }
    if isinstance(x, DataArray):
        return _transform_data_array(x, coords=coords, graph=graph, options=options)
    else:
        return _transform_dataset(x, coords=coords, graph=graph, options=options)


def show_graph(graph: GraphDict, size: str = None, simplified: bool = False):
    """
    Show graphical representation of a graph as required by
    :py:func:`transform_coords`

    Requires `python-graphviz` package.

    :param graph: Transformation graph to show.
    :param size: Size forwarded to graphviz, must be a string, "width,height"
                 or "size". In the latter case, the same value is used for
                 both width and height.
    :param simplified: If ``True``, do not show the conversion functions,
                       only the potential input and output coordinates.
    """
    return Graph(graph).show(size=size, simplified=simplified)
