# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

from __future__ import annotations

from graphlib import TopologicalSorter
from typing import Callable, Dict, Iterable, List, Set, Tuple, Union

from ..core import DataArray
from .rule import ComputeRule, FetchRule, RenameRule, Rule

GraphDict = Dict[Union[str, Tuple[str, ...]], Union[str, Callable]]


class Graph:
    def __init__(self, graph: Union[GraphDict, Dict[str, Rule]]):
        if not graph:
            self._rules = {}
        elif isinstance(next(iter(graph.values())), Rule):
            self._rules: Dict[str, Rule] = graph
        else:
            self._rules: Dict[str, Rule] = _convert_to_rule_graph(graph)

    def __getitem__(self, name: str) -> Rule:
        return self._rules[name]

    def items(self) -> Iterable[Tuple[str, Rule]]:
        yield from self._rules.items()

    def parents_of(self, node: str) -> Iterable[str]:
        try:
            yield from self._rules[node].dependencies
        except KeyError:
            # Input nodes have no parents but are not represented in the
            # graph unless the corresponding FetchRules have been added.
            return

    def children_of(self, node: str) -> Iterable[str]:
        for candidate, rule in self.items():
            if node in rule.dependencies:
                yield candidate

    def nodes(self) -> Iterable[str]:
        yield from self._rules.keys()

    def nodes_topologically(self) -> Iterable[str]:
        yield from TopologicalSorter(
            {out: rule.dependencies
             for out, rule in self._rules.items()}).static_order()

    def graph_for(self, da: DataArray, targets: Set[str]) -> Graph:
        """
        Construct a graph containing only rules needed for the given DataArray
        and targets, including FetchRules for the inputs.
        """
        subgraph = {}
        depth_first_stack = list(targets)
        while depth_first_stack:
            out_name = depth_first_stack.pop()
            if out_name in subgraph:
                continue
            rule = self._rule_for(out_name, da)
            for name in rule.out_names:
                subgraph[name] = rule
            depth_first_stack.extend(rule.dependencies)
        return Graph(subgraph)

    def _rule_for(self, out_name: str, da: DataArray) -> Rule:
        if _is_in_meta_data(out_name, da):
            return FetchRule((out_name, ), da.meta, da.bins.meta if da.bins else {})
        try:
            return self._rules[out_name]
        except KeyError:
            raise KeyError(f"Coordinate '{out_name}' does not exist in the input data "
                           "and no rule has been provided to compute it.") from None

    def show(self, size=None, simplified=False):
        dot = _make_graphviz_digraph(strict=True)
        dot.attr('node', shape='box', height='0.1')
        dot.attr(size=size)
        for output, rule in self._rules.items():
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
                       map(lambda n: rules[n], rules.nodes_topologically())):
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
