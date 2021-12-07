# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

from __future__ import annotations

from copy import deepcopy
import dataclasses
from graphlib import TopologicalSorter
from itertools import islice
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

    @classmethod
    def make_empty(cls):
        return cls(nodes=set(), inputs=set(), outputs=set())


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

    def undirected_cycles(self, n=None) -> Set[Cycle]:
        cycles = set()
        # It is enough to only start from rule outputs, because there are no cycles
        # that include only inputs.
        for start in self._child_to_parent.keys():
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
                    dfs.push([path + [neighbor]])
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
            # No intermediate nodes that can be contracted.
            # This prevents (among others) a->b from being detected as a cycle
            # but allows a<->b (2 edges).
            return
        cycles.add(Cycle(nodes=set(path_section), inputs=inputs, outputs=outputs))

    def add_node(self, name, parents):
        self._child_to_parent[name] = parents

    def remove_node(self, name: str):
        self._child_to_parent.pop(name, None)
        for other in self.children_of(name):
            self._child_to_parent[other].discard(name)

    def add_parent(self, child: str, parent: str):
        self._child_to_parent[child].add(parent)

    def remove_parent(self, child: str, parent: str):
        self._child_to_parent[child].discard(parent)

    def contract_cycle(self, cycle: Cycle) -> Graph:
        intermediates = list(cycle.intermediates())
        new_node = _make_new_node_name(intermediates)
        work_graph = deepcopy(self)

        for node in intermediates:
            work_graph.remove_node(node)

        new_parents = set(cycle.inputs)
        for node in intermediates:
            for parent in self.parents_of(node):
                if parent not in intermediates:
                    new_parents.add(parent)
        work_graph.add_node(new_node, new_parents)

        for node in self._child_to_parent:
            if node not in cycle.inputs and node not in intermediates:
                for parent in self.parents_of(node):
                    if parent in cycle.inputs:
                        work_graph.remove_parent(node, parent)
                    if parent in cycle.inputs or parent in intermediates:
                        work_graph.add_parent(node, new_node)

        return work_graph

    def fully_contract_cycles(self) -> Graph:
        graph = self
        cycles = graph.undirected_cycles(n=1)
        while cycles:
            cycle = next(iter(cycles))
            graph = graph.contract_cycle(cycle)
            cycles = graph.undirected_cycles(n=1)
        return graph


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
            subgraph[out_name] = rule
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
    for rule in filter(lambda r: r not in already_used,
                       map(lambda n: rules[n], _sort_topologically(rules))):
        already_used.add(rule)
        result.append(rule)
    return result


def _sort_topologically(graph: RuleGraph) -> Iterable[str]:
    yield from TopologicalSorter(
        {out: rule.dependencies
         for out, rule in graph.items()}).static_order()


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


def _make_new_node_name(node_names: Iterable[str]) -> str:
    cycle_prefix = '__SC_CYCLE_NODE:'
    return cycle_prefix + ':'.join(
        name.replace(cycle_prefix, '') for name in node_names)


def _make_graphviz_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError:
        raise RuntimeError('Failed to import `graphviz`, please install `graphviz` if '
                           'using `pip`, or `python-graphviz` if using `conda`.')
    return Digraph(*args, **kwargs)
