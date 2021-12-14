# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

from __future__ import annotations

from graphlib import TopologicalSorter
from typing import Callable, Dict, Iterable, List, Set, Tuple, Union

from ..core import DataArray, NotFoundError
from .rule import ComputeRule, FetchRule, RenameRule, Rule

GraphDict = Dict[Union[str, Tuple[str, ...]], Union[str, Callable]]


class DepthFirstSearch:
    def __init__(self, initial: Iterable):
        self._stack = list(initial)
        self._iterating = False

    def __iter__(self):
        assert not self._iterating
        self._iterating = True
        return self

    def __next__(self):
        assert self._iterating
        if self._stack:
            return self._stack.pop()
        self._iterating = False
        raise StopIteration

    def push(self, next_values: Iterable):
        self._stack.extend(next_values)


class Graph:
    def __init__(self, graph: Dict[str, Iterable[str]]):
        self._child_to_parent = {key: set(values) for key, values in graph.items()}

    def __eq__(self, other: Graph) -> bool:
        return self._child_to_parent == other._child_to_parent

    def __repr__(self):
        return str(self)

    def __getitem__(self, node: str):
        return self._child_to_parent[node]

    def parents_of(self, node: str) -> Iterable[str]:
        try:
            yield from self._child_to_parent[node]
        except KeyError:
            # Input nodes have no parents but are not represented in the
            # graph unless the corresponding FetchRules have been added.
            return

    def children_of(self, node: str) -> Iterable[str]:
        for candidate, parents in self._child_to_parent.items():
            if node in parents:
                yield candidate

    def neighbors_of(self, node: str) -> Iterable[str]:
        yield from self.parents_of(node)
        yield from self.children_of(node)

    def roots(self) -> Iterable[str]:
        yield from (name for name, parents in self._child_to_parent.items()
                    if not parents)

    def nodes(self) -> Iterable[str]:
        yield from self._child_to_parent.keys()

    def nodes_topologically(self) -> Iterable[str]:
        yield from TopologicalSorter(self._child_to_parent).static_order()


class RuleGraph:
    def __init__(self, graph: Union[GraphDict, Dict[str, Rule]]):
        if isinstance(next(iter(graph.values())), Rule):
            self._rule_graph: Dict[str, Rule] = graph
        else:
            self._rule_graph: Dict[str, Rule] = _convert_to_rule_graph(graph)
        self.dependency_graph = Graph(
            {output: rule.dependencies
             for output, rule in self._rule_graph.items()})

    def __getitem__(self, name: str) -> Rule:
        return self._rule_graph[name]

    def items(self) -> Iterable[Tuple[str, Rule]]:
        yield from self._rule_graph.items()

    def graph_for(self, da: DataArray, targets: Set[str]) -> RuleGraph:
        """
        Construct a graph containing only rules needed for the given DataArray
        and targets, including FetchRules for the inputs.
        """
        subgraph = {}
        dfs = DepthFirstSearch(targets)
        for out_name in dfs:
            if out_name in subgraph:
                continue
            rule = self._rule_for(out_name, da)
            for name in rule.out_names:
                subgraph[name] = rule
            dfs.push(rule.dependencies)
        return RuleGraph(subgraph)

    def _rule_for(self, out_name: str, da: DataArray) -> Rule:
        if _is_in_meta_data(out_name, da):
            return FetchRule((out_name, ), da.meta, da.bins.meta if da.bins else {})
        try:
            return self._rule_graph[out_name]
        except KeyError:
            raise NotFoundError(
                f"Coordinate '{out_name}' does not exist in the input data "
                "and no rule has been provided to compute it.") from None

    def show(self, size=None, simplified=False):
        dot = _make_graphviz_digraph(strict=True)
        dot.attr('node', shape='box', height='0.1')
        dot.attr(size=size)
        for output, rule in self._rule_graph.items():
            if isinstance(rule, RenameRule):
                dot.edge(rule.dependencies[0], output, style='dashed')
            elif isinstance(rule, ComputeRule):
                if not simplified:
                    name = f'{rule.func_name}(...)'
                    dot.node(name, shape='ellipse', style='filled', color='lightgrey')
                    dot.edge(name, output)
                else:
                    name = output
                for arg in rule.dependencies:
                    dot.edge(arg, name)
        return dot


def rule_sequence(rules: RuleGraph) -> List[Rule]:
    already_used = set()
    result = []
    for rule in filter(
            lambda r: r not in already_used,
            map(lambda n: rules[n], rules.dependency_graph.nodes_topologically())):
        already_used.add(rule)
        result.append(rule)
    return result


def _make_rule(products, producer) -> Rule:
    if isinstance(producer, str):
        return RenameRule(products, producer)
    return ComputeRule(products, producer)


def _convert_to_rule_graph(graph: GraphDict) -> Dict[str, Rule]:
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


def _is_in_meta_data(name: str, da: DataArray) -> bool:
    return name in da.meta or (da.bins is not None and name in da.bins.meta)


def _make_graphviz_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError:
        raise RuntimeError('Failed to import `graphviz`, please install `graphviz` if '
                           'using `pip`, or `python-graphviz` if using `conda`.')
    return Digraph(*args, **kwargs)
