# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

from __future__ import annotations

from abc import ABC, abstractmethod
from copy import copy
import dataclasses
from enum import Enum, auto
from graphlib import TopologicalSorter
import inspect
from typing import Any, Callable, Dict, Iterable, List, Mapping,\
    Optional, Set, Tuple, Union

from .core import DataArray, Dataset, NotFoundError, VariableError,\
    Variable, bins
from .logging import get_logger

_OptionalCoordTuple = Tuple[Optional[Variable], Optional[Variable]]
GraphDict = Dict[Union[str, Tuple[str, ...]], Union[str, Callable]]


@dataclasses.dataclass(frozen=True)
class _Options:
    rename_dims: bool
    keep_aliases: bool
    keep_intermediate: bool
    keep_inputs: bool


class _Destination(Enum):
    coord = auto()
    attr = auto()


@dataclasses.dataclass
class _Coord:
    dense: Variable  # for dense variable or bin-coord
    event: Variable
    destination: _Destination
    usages: int = -1  # negative for unlimited usages

    @property
    def has_dense(self) -> bool:
        return self.dense is not None

    @property
    def has_event(self) -> bool:
        return self.event is not None

    def use(self):
        if self.usages > 0:
            self.usages -= 1

    @property
    def used_up(self) -> bool:
        return self.usages == 0


class _CoordTable:
    def __init__(self, rules: List[_Rule], targets: Set[str], options: _Options):
        self._coords = {}
        self._total_usages = _apply_keep_options(_count_usages(rules), rules, targets,
                                                 options)
        self._options = options

    def add(self, name: str, coord: _Coord):
        self._coords[name] = dataclasses.replace(coord, usages=self.total_usages(name))

    def consume(self, name: str) -> _Coord:
        coord = self._coords[name]
        coord.destination = _Destination.attr
        coord.use()
        if coord.used_up:
            del self._coords[name]
        return coord

    def total_usages(self, name: str) -> int:
        return self._total_usages.get(name, -1)

    def items(self) -> Iterable[Tuple[str, _Coord]]:
        yield from self._coords.items()


def _count_usages(rules: List[_Rule]) -> Dict[str, int]:
    usages = {}
    for rule in rules:
        for name in rule.dependencies:
            usages.setdefault(name, 0)
            usages[name] += 1
    return usages


def _apply_keep_options(usages: Dict[str, int], rules: List[_Rule], targets: Set[str],
                        options: _Options) -> Dict[str, int]:
    def out_names(rule_type):
        yield from filter(lambda name: name not in targets,
                          _rule_output_names(rules, rule_type))

    def handle_in(names):
        for name in names:
            usages[name] = -1

    inputs = set(out_names(_FetchRule))
    aliases = set(out_names(_RenameRule))
    all_inputs = set(dep for rule in rules for dep in rule.dependencies)
    if options.keep_inputs:
        handle_in(inputs)
    if options.keep_intermediate:
        handle_in(all_inputs - inputs - aliases)
    if options.keep_aliases:
        handle_in(aliases)
    return usages


class _Rule(ABC):
    def __init__(self, out_names: Tuple[str, ...]):
        self.out_names = out_names

    @abstractmethod
    def __call__(self, coords: _CoordTable) -> Dict[str, Variable]:
        """Evaluate the kernel."""

    @property
    @abstractmethod
    def dependencies(self) -> Tuple[str]:
        """Return names of coords that this kernel needs as inputs."""

    def _format_out_names(self):
        return f'({", ".join(self.out_names)})'


class _FetchRule(_Rule):
    def __init__(self, out_names: Tuple[str, ...], dense_sources: Mapping[str,
                                                                          Variable],
                 event_sources: Mapping[str, Variable]):
        super().__init__(out_names)
        self._dense_sources = dense_sources
        self._event_sources = event_sources

    def __call__(self, coords: _CoordTable) -> Dict[str, _Coord]:
        return {
            out_name: _Coord(dense=self._dense_sources.get(out_name, None),
                             event=self._event_sources.get(out_name, None),
                             destination=_Destination.coord)
            for out_name in self.out_names
        }

    @property
    def dependencies(self) -> Tuple[str]:
        return ()  # type: ignore

    def __str__(self):
        return f'Input   {self._format_out_names()}'


class _RenameRule(_Rule):
    def __init__(self, out_names: Tuple[str, ...], in_name: str):
        super().__init__(out_names)
        self._in_name = in_name

    def __call__(self, coords: _CoordTable) -> Dict[str, _Coord]:
        # Shallow copy the _Coord object to allow the alias to have
        # a different destination and usage count than the original.
        return {
            out_name: copy(coords.consume(self._in_name))
            for out_name in self.out_names
        }

    @property
    def dependencies(self) -> Tuple[str]:
        return tuple((self._in_name, ))

    def __str__(self):
        return f'Rename  {self._format_out_names()} <- {self._in_name}'


def _arg_names(func) -> Tuple[str]:
    spec = inspect.getfullargspec(func)
    if spec.varargs is not None or spec.varkw is not None:
        raise ValueError('Function with variable arguments not allowed in '
                         f'conversion graph: `{func.__name__}`.')
    return tuple(spec.args + spec.kwonlyargs)


class _ComputeRule(_Rule):
    def __init__(self, out_names: Tuple[str, ...], func: Callable):
        super().__init__(out_names)
        self._func = func
        self._arg_names = _arg_names(func)

    def __call__(self, coords: _CoordTable) -> Dict[str, _Coord]:
        inputs = {name: coords.consume(name) for name in self._arg_names}
        outputs = None
        if any(coord.has_event for coord in inputs.values()):
            outputs = self._compute_with_events(inputs)
        if all(coord.has_dense for coord in inputs.values()):
            dense_outputs = self._compute_pure_dense(inputs)
            if outputs is None:
                outputs = dense_outputs
            else:
                for name, coord in dense_outputs.items():
                    outputs[name].dense = coord.dense
        return self._without_unrequested(outputs)

    def _compute_pure_dense(self, inputs):
        outputs = self._func(**{name: coord.dense for name, coord in inputs.items()})
        outputs = self._to_dict(outputs)
        return {
            name: _Coord(dense=var, event=None, destination=_Destination.coord)
            for name, var in outputs.items()
        }

    def _compute_with_events(self, inputs):
        args = {
            name: coord.event if coord.has_event else coord.dense
            for name, coord in inputs.items()
        }
        outputs = self._to_dict(self._func(**args))
        # Dense outputs may be produced as side effects of processing event
        # coords.
        outputs = {
            name: _Coord(dense=var if var.bins is None else None,
                         event=var if var.bins is not None else None,
                         destination=_Destination.coord)
            for name, var in outputs.items()
        }
        return outputs

    def _without_unrequested(self, d: Dict[str, Any]) -> Dict[str, Any]:
        return {key: val for key, val in d.items() if key in self.out_names}

    def _to_dict(self, output) -> Dict[str, Variable]:
        if not isinstance(output, dict):
            if len(self.out_names) != 1:
                raise TypeError('Function returned a single output but '
                                f'{len(self.out_names)} were expected.')
            return {self.out_names[0]: output}
        return output

    @property
    def dependencies(self) -> Tuple[str]:
        return self._arg_names

    @property
    def func_name(self) -> str:
        return self._func.__name__

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
        products = (products, ) if isinstance(products, str) else tuple(products)
        rule = _make_rule(products, producer)
        for product in products:
            if product in rule_graph:
                raise ValueError(
                    f'Duplicate output name defined in conversion graph: {product}')
            rule_graph[product] = rule
    return rule_graph


def _sort_topologically(graph: Mapping[str, _Rule]) -> Iterable[str]:
    yield from TopologicalSorter(
        {out: rule.dependencies
         for out, rule in graph.items()}).static_order()


def _is_meta_data(name: str, da: DataArray) -> bool:
    return name in da.meta or (da.bins is not None and name in da.bins.meta)


def _make_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError:
        raise RuntimeError('Failed to import `graphviz`, please install `graphviz` if '
                           'using `pip`, or `python-graphviz` if using `conda`.')
    return Digraph(*args, **kwargs)


class Graph:
    def __init__(self, graph):
        self._graph = _convert_to_rule_graph(graph)

    def subgraph(self, da: DataArray, targets: Set[str]) -> Dict[str, _Rule]:
        subgraph = {}
        pending = list(targets)
        while pending:
            out_name = pending.pop()
            if out_name in subgraph:
                continue
            rule = self._rule_for(out_name, da)
            subgraph[out_name] = rule
            pending.extend(rule.dependencies)
        return subgraph

    def _rule_for(self, out_name: str, da: DataArray) -> _Rule:
        if _is_meta_data(out_name, da):
            return _FetchRule((out_name, ), da.meta, da.bins.meta if da.bins else {})
        try:
            return self._graph[out_name]
        except KeyError:
            raise NotFoundError(
                f"Coordinate '{out_name}' does not exist in the input data "
                "and no rule has been provided to compute it.") from None

    def show(self, size=None, simplified=False):
        dot = _make_digraph(strict=True)
        dot.attr('node', shape='box', height='0.1')
        dot.attr(size=size)
        for output, rule in self._graph.items():
            if isinstance(rule, _RenameRule):
                dot.edge(rule.dependencies[0], output, style='dashed')
            elif isinstance(rule, _ComputeRule):
                if not simplified:
                    name = f'{rule.func_name}(...)'
                    dot.node(name, shape='ellipse', style='filled', color='lightgrey')
                    dot.edge(name, output)
                else:
                    name = output
                for arg in rule.dependencies:
                    dot.edge(arg, name)
        return dot


def _log_plan(rules: List[_Rule], targets: Set[str],
              dim_name_changes: Mapping[str, str], coords: _CoordTable) -> None:
    inputs = set(_rule_output_names(rules, _FetchRule))
    byproducts = {
        name
        for name in (set(_rule_output_names(rules, _RenameRule))
                     | set(_rule_output_names(rules, _ComputeRule))) - targets
        if coords.total_usages(name) < 0
    }

    message = f'Transforming coords ({", ".join(sorted(inputs))}) ' \
              f'-> ({", ".join(sorted(targets))})'
    if byproducts:
        message += f'\n  Byproducts:\n    {", ".join(sorted(byproducts))}'
    if dim_name_changes:
        dim_rename_steps = '\n'.join(f'    {t} <- {f}'
                                     for f, t in dim_name_changes.items())
        message += '\n  Rename dimensions:\n' + dim_rename_steps
    message += '\n  Steps:\n' + '\n'.join(
        f'    {rule}' for rule in rules if not isinstance(rule, _FetchRule))

    get_logger().info(message)


def _store_coord(da: DataArray, name: str, coord: _Coord) -> None:
    def out(x):
        return x.coords if coord.destination == _Destination.coord else x.attrs

    def del_other(x):
        try:
            if coord.destination == _Destination.coord:
                del x.attrs[name]
            else:
                del x.coords[name]
        except NotFoundError:
            pass

    if coord.has_dense:
        out(da)[name] = coord.dense
        del_other(da)
    if coord.has_event:
        try:
            out(da.bins)[name] = coord.event
        except VariableError:
            # Thrown on mismatching bin indices, e.g. slice
            da.data = da.data.copy()
            out(da.bins)[name] = coord.event
        del_other(da.bins)


def _store_results(da: DataArray, coords: _CoordTable, targets: Set[str]) -> DataArray:
    da = da.copy(deep=False)
    if da.bins is not None:
        da.data = bins(**da.bins.constituents)
    for name, coord in coords.items():
        if name in targets:
            coord.destination = _Destination.coord
        _store_coord(da, name, coord)
    return da


def _rules_of_type(rules: List[_Rule], rule_type: type) -> Iterable[_Rule]:
    yield from filter(lambda rule: isinstance(rule, rule_type), rules)


def _rule_output_names(rules: List[_Rule], rule_type: type) -> Iterable[str]:
    for rule in _rules_of_type(rules, rule_type):
        yield from rule.out_names


def _initial_dims_to_rename(da: DataArray, rules: List[_Rule]) -> Dict[str, List[str]]:
    res = {}
    for rule in filter(lambda r: isinstance(r, _FetchRule), rules):
        for name in rule.out_names:
            if name in da.dims:
                res[name] = []
    return res


def _rules_with_dep(dep, rules):
    return list(filter(lambda r: dep in r.dependencies, rules))


# A coord's dim can be renamed if its node
#  1. has one incoming dim coord
#  2. has only one outgoing connection
#
# This functions traversed the graph in depth-first order
# and builds a dict of old->new names according to the conditions above.
def _dim_name_changes(rules: List[_Rule], dims: List[str]) -> Dict[str, str]:
    dim_coords = tuple(name for name in _rule_output_names(rules, _FetchRule)
                       if name in dims)
    pending = list(dim_coords)
    incoming_dim_coords = {name: [name] for name in pending}
    name_changes = {}
    while pending:
        name = pending.pop(0)
        if len(incoming_dim_coords[name]) != 1:
            continue  # Condition 1.
        dim = incoming_dim_coords[name][0]
        name_changes[dim] = name
        outgoing = _rules_with_dep(name, rules)
        if len(outgoing) == 1 and len(outgoing[0].out_names) == 1:
            # Potential candidate according to condition 2.
            pending.append(outgoing[0].out_names[0])
        for rule in filter(lambda r: len(r.out_names) == 1, outgoing):
            # Condition 2. is not satisfied for these children but we
            # still need to take the current node into account for 1.
            incoming_dim_coords.setdefault(rule.out_names[0], []).append(dim)
    return name_changes


def _transform_data_array(original: DataArray, targets: Set[str], graph: Graph,
                          options: _Options) -> DataArray:
    rules = _non_duplicate_rules(graph.subgraph(original, targets))
    dim_name_changes = (_dim_name_changes(rules, original.dims)
                        if options.rename_dims else {})
    working_coords = _CoordTable(rules, targets, options)
    _log_plan(rules, targets, dim_name_changes, working_coords)
    for rule in rules:
        for name, coord in rule(working_coords).items():
            working_coords.add(name, coord)

    res = _store_results(original, working_coords, targets)
    return res.rename_dims(dim_name_changes)


def _transform_dataset(original: Dataset, targets: Set[str], graph: Graph, *,
                       options: _Options) -> Dataset:
    # Note the inefficiency here in datasets with multiple items: Coord
    # transform is repeated for every item rather than sharing what is
    # possible. Implementing this would be tricky and likely error-prone,
    # since different items may have different attributes. Unless we have
    # clear performance requirements we therefore go with the safe and
    # simple solution
    return Dataset(
        data={
            name: _transform_data_array(
                original[name], targets=targets, graph=graph, options=options)
            for name in original
        })


def transform_coords(x: Union[DataArray, Dataset],
                     targets: Union[str, Iterable[str]],
                     graph: GraphDict,
                     *,
                     rename_dims: bool = True,
                     keep_aliases: bool = True,
                     keep_intermediate: bool = True,
                     keep_inputs: bool = True) -> Union[DataArray, Dataset]:
    """Compute new coords based on transformations of input coords.

    :param x: Input object with coords.
    :param targets: Name or list of names of desired output coords.
    :param graph: A graph defining how new coords can be computed from existing
                  coords. This may be done in multiple steps.
                  The graph is given by a ``dict`` where:

                  - Dict keys are ``str`` or ``tuple`` of ``str``, defining the
                    names of outputs generated by a dict value.
                  - Dict values are ``str`` or a callable (function). If ``str``,
                    this is a synonym for renaming a coord. If a callable,
                    it must either return a single variable or a dict of
                    variables. The argument names of callables must be coords
                    in ``x`` or be computable by other nodes in ``graph``.
    :param rename_dims: Rename dimensions if the corresponding dimension coords
                        are used as inputs. `Dimension` ``a`` is renamed to ``b``
                        if and only if

                        - `coord` ``a`` is used as input in exactly 1 node
                        - `coord` ``a`` is the only dimension coord in
                          that node's inputs
                        - `coord` ``b`` is the only output of that node.

                        Default is True.
    :param keep_aliases: If True, aliases for coords defined in graph are
                         included in the output. Default is True.
    :param keep_intermediate: Keep attributes created as intermediate results.
                              Default is True.
    :param keep_inputs: Keep consumed input coordinates or attributes.
                        Default is True.
    :return: New object with desired coords. Existing data and meta-data is
             shallow-copied.

    :seealso: The section in the user guide on
     `Coordinate transformations <../../user-guide/coordinate-transformations.rst>`_
    """
    options = _Options(rename_dims=rename_dims,
                       keep_aliases=keep_aliases,
                       keep_intermediate=keep_intermediate,
                       keep_inputs=keep_inputs)
    targets = {targets} if isinstance(targets, str) else set(targets)
    if isinstance(x, DataArray):
        return _transform_data_array(x,
                                     targets=targets,
                                     graph=Graph(graph),
                                     options=options)
    else:
        return _transform_dataset(x,
                                  targets=targets,
                                  graph=Graph(graph),
                                  options=options)


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
