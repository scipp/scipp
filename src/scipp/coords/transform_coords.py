# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

from itertools import takewhile
from typing import Dict, Iterable, List, Mapping, Optional, Set, Union

from ..core import DataArray, Dataset, NotFoundError, VariableError, bins
from ..logging import get_logger
from .coord_table import Coord, CoordTable, Destination
from .graph import Graph, GraphDict, RuleGraph, is_cycle_node,\
    is_in_cycle, rule_sequence
from .options import Options
from .rule import ComputeRule, FetchRule, RenameRule, Rule, rule_output_names


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
    options = Options(rename_dims=rename_dims,
                      keep_aliases=keep_aliases,
                      keep_intermediate=keep_intermediate,
                      keep_inputs=keep_inputs)
    targets = {targets} if isinstance(targets, str) else set(targets)
    if isinstance(x, DataArray):
        return _transform_data_array(x,
                                     targets=targets,
                                     graph=RuleGraph(graph),
                                     options=options)
    else:
        return _transform_dataset(x,
                                  targets=targets,
                                  graph=RuleGraph(graph),
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
    return RuleGraph(graph).show(size=size, simplified=simplified)


def _transform_data_array(original: DataArray, targets: Set[str], graph: RuleGraph,
                          options: Options) -> DataArray:
    graph = graph.graph_for(original, targets)
    rules = rule_sequence(graph)
    dim_name_changes = (_dim_name_changes(graph, original.dims, options)
                        if options.rename_dims else {})
    working_coords = CoordTable(rules, targets, options)
    _log_plan(rules, targets, dim_name_changes, working_coords)
    for rule in rules:
        for name, coord in rule(working_coords).items():
            working_coords.add(name, coord)

    res = _store_results(original, working_coords, targets)
    return res.rename_dims(dim_name_changes)


def _transform_dataset(original: Dataset, targets: Set[str], graph: RuleGraph, *,
                       options: Options) -> Dataset:
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


def _log_plan(rules: List[Rule], targets: Set[str], dim_name_changes: Mapping[str, str],
              coords: CoordTable) -> None:
    inputs = set(rule_output_names(rules, FetchRule))
    byproducts = {
        name
        for name in (set(rule_output_names(rules, RenameRule))
                     | set(rule_output_names(rules, ComputeRule))) - targets
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
        f'    {rule}' for rule in rules if not isinstance(rule, FetchRule))

    get_logger().info(message)


def _store_coord(da: DataArray, name: str, coord: Coord) -> None:
    def out(x):
        return x.coords if coord.destination == Destination.coord else x.attrs

    def del_other(x):
        try:
            if coord.destination == Destination.coord:
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


def _store_results(da: DataArray, coords: CoordTable, targets: Set[str]) -> DataArray:
    da = da.copy(deep=False)
    if da.bins is not None:
        da.data = bins(**da.bins.constituents)
    for name, coord in coords.items():
        if name in targets:
            coord.destination = Destination.coord
        _store_coord(da, name, coord)
    return da


def _has_single_element(iterable: Iterable) -> bool:
    n = 0
    for _ in iterable:
        n += 1
        if n > 1:
            return False
    return n == 1


def _dim_associations_of_cycle_node(node: str, dims: List[str]) -> Set[str]:
    return {dim for dim in dims if is_in_cycle(dim, node)}


def _dim_associations_of_parents(node: str, graph: Graph,
                                 associated_dims: Dict[str, Set[str]],
                                 is_split_node: Dict[str, bool]) -> Set[str]:
    dim_parents = set()
    for parent in graph.parents_of(node):
        if is_split_node[parent]:
            # If *any* parent is a split node, this node cannot
            # be associated with any parent dim.
            return set()
        dim_parents.update(associated_dims[parent])
    return dim_parents if len(dim_parents) == 1 else set()


def _associate_nodes_with_dim_coords(graph: Graph,
                                     dims: List[str]) -> Dict[str, Optional[str]]:
    associated_dims: Dict[str, Set[str]] = {}
    is_split_node: Dict[str, bool] = {}

    for node in graph.nodes_topologically():
        is_split_node[node] = not _has_single_element(graph.children_of(node))
        if node in dims:
            associated_dims[node] = {node}
        else:
            associated_dims[node] = _dim_associations_of_parents(
                node, graph, associated_dims, is_split_node)
            if is_cycle_node(node):
                associated_dims[node].update(_dim_associations_of_cycle_node(
                    node, dims))

    return {
        name: next(iter(d)) if len(d) == 1 else None
        for name, d in associated_dims.items()
    }


def _dim_coords(graph: Graph, dims: List[str]) -> Iterable[str]:
    for dim in dims:
        for node in graph.nodes():
            if dim == node:
                yield dim, node
                break
            if is_in_cycle(dim, node):
                yield dim, node
                break


def _is_valid_rename_candidate(node: str, rule_graph: RuleGraph,
                               options: Options) -> bool:
    return (not is_cycle_node(node) and
            (options.keep_intermediate or not isinstance(rule_graph[node], ComputeRule))
            and (options.keep_aliases or not isinstance(rule_graph[node], RenameRule)))


def _walk_single_children(graph, start):
    node = start
    children = list(graph.children_of(node))
    while len(children) == 1:
        node = children[0]
        yield node
        children = list(graph.children_of(node))


def _dim_name_changes(rule_graph: RuleGraph, dims: List[str],
                      options: Options) -> Dict[str, str]:
    graph = rule_graph.dependency_graph.fully_contract_cycles()
    associated_dims = _associate_nodes_with_dim_coords(graph, dims)

    name_changes = {}
    for dim_coord, node in _dim_coords(graph, dims):
        candidate = node if _is_valid_rename_candidate(node, rule_graph,
                                                       options) else None
        for child in takewhile(lambda c: associated_dims[c] == dim_coord,
                               _walk_single_children(graph, node)):
            if _is_valid_rename_candidate(child, rule_graph, options):
                candidate = child
        if candidate is not None and candidate != dim_coord:
            name_changes[dim_coord] = candidate

    return name_changes
