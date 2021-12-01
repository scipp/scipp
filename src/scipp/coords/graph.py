# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

from __future__ import annotations

import dataclasses
from graphlib import TopologicalSorter
from itertools import islice
from typing import Callable, Dict, Iterable, List, Set, Tuple, Union

from ..core import DataArray, NotFoundError
from .rule import ComputeRule, FetchRule, RenameRule, Rule

GraphDict = Dict[Union[str, Tuple[str, ...]], Union[str, Callable]]
RuleGraph = Dict[str, Rule]


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


@dataclasses.dataclass
class Cycle:
    nodes: Set[str]
    inputs: Set[str]
    outputs: Set[str]

    def __hash__(self):
        # For a given graph, the nodes uniquely identify the cycle.
        # There is no need to include inputs and outputs in the hash.
        return hash(tuple(sorted(self.nodes)))

    def intermediates(self):
        yield from (node for node in self.nodes
                    if node not in self.inputs and node not in self.outputs)


class Graph:
    def __init__(self, graph: Union[GraphDict, RuleGraph]):
        if isinstance(next(iter(graph.values())), Rule):
            self._rule_graph = graph
        else:
            self._rule_graph = _convert_to_rule_graph(graph)

    def __getitem__(self, item) -> Rule:
        return self._rule_graph[item]

    def items(self) -> Iterable[Tuple[str, Rule]]:
        yield from self._rule_graph.items()

    def parents_of(self, node: str) -> Iterable[str]:
        try:
            yield from self._rule_graph[node].dependencies
        except KeyError:
            # Input nodes have no parents but are not represented in the
            # graph unless the corresponding FetchRules have been added.
            return

    def children_of(self, node: str) -> Iterable[str]:
        for candidate, rule in self._rule_graph.items():
            if node in rule.dependencies:
                yield candidate

    def neighbors_of(self, node: str) -> Iterable[str]:
        yield from self.parents_of(node)
        yield from self.children_of(node)

    def graph_for(self, da: DataArray, targets: Set[str]) -> Graph:
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
            subgraph[out_name] = rule
            dfs.push(rule.dependencies)
        return Graph(subgraph)

    def _rule_for(self, out_name: str, da: DataArray) -> Rule:
        if _is_in_meta_data(out_name, da):
            return FetchRule((out_name, ), da.meta, da.bins.meta if da.bins else {})
        try:
            return self._rule_graph[out_name]
        except KeyError:
            raise NotFoundError(
                f"Coordinate '{out_name}' does not exist in the input data "
                "and no rule has been provided to compute it.") from None

    def undirected_cycles(self, n=None) -> Set[Cycle]:
        cycles = set()
        # It is enough to only start from rule outputs, because there are no cycles
        # that include only inputs.
        for start in self._rule_graph.keys():
            cycles.update(self._undirected_cycles_from(start, n))
            if n is not None and len(cycles) >= n:
                return set(islice(cycles, 0, n))
        return cycles

    def _undirected_cycles_from(self, start, n) -> Set[Cycle]:
        dfs = DepthFirstSearch([[start]])
        cycles = set()
        for path in dfs:
            head = path[-1]
            for neighbor in self.neighbors_of(head):
                if neighbor in path:
                    self._add_cycle_to(cycles, path[path.index(neighbor):])
                    if n is not None and len(cycles) >= n:
                        return set(islice(cycles, 0, n))
                else:
                    dfs.push(path + [neighbor])
        return cycles

    def _add_cycle_to(self, cycles, path_section):
        inputs = set()
        outputs = set()
        inner = set()
        for node in path_section:
            has_parents_in_graph = any(parent in path_section
                                       for parent in self.parents_of(node))
            has_children_in_graph = any(child in path_section
                                        for child in self.children_of(node))
            if has_parents_in_graph:
                if not has_children_in_graph:
                    outputs.add(node)
                else:
                    inner.add(node)
            else:
                # Must have outgoing edges.
                inputs.add(node)
        if not inner:
            return  # No intermediate nodes that can be contracted.
        cycles.add(Cycle(nodes=set(path_section), inputs=inputs, outputs=outputs))

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


def rule_sequence(rules: Graph) -> List[Rule]:
    already_used = set()
    result = []
    for rule in filter(lambda r: r not in already_used,
                       map(lambda n: rules[n], _sort_topologically(rules))):
        already_used.add(rule)
        result.append(rule)
    return result


def _sort_topologically(graph: Graph) -> Iterable[str]:
    yield from TopologicalSorter(
        {out: rule.dependencies
         for out, rule in graph.items()}).static_order()


def _make_rule(products, producer) -> Rule:
    if isinstance(producer, str):
        return RenameRule(products, producer)
    return ComputeRule(products, producer)


def _convert_to_rule_graph(graph: GraphDict) -> RuleGraph:
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
