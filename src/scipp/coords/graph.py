# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

from __future__ import annotations

import collections
from collections.abc import Iterable
from graphlib import TopologicalSorter
from typing import TYPE_CHECKING, Any

from ..core import DataArray
from ..utils.graph import make_graphviz_digraph
from .rule import ComputeRule, FetchRule, Kernel, RenameRule, Rule

if TYPE_CHECKING:
    try:
        from graphviz import Digraph
    except ImportError:
        Digraph = Any
else:
    Digraph = object

GraphDict = dict[str | tuple[str, ...], str | Kernel]


class Graph:
    def __init__(self, graph: GraphDict | dict[str, Rule]):
        if not isinstance(graph, collections.abc.Mapping):
            raise TypeError("'graph' must be a dict")
        if not graph:
            self._rules: dict[str, Rule] = {}
        elif isinstance(next(iter(graph.values())), Rule):
            self._rules = graph  # type: ignore[assignment]
        else:
            self._rules = _convert_to_rule_graph(graph)  # type: ignore[arg-type]

    def __getitem__(self, name: str) -> Rule:
        return self._rules[name]

    def items(self) -> Iterable[tuple[str, Rule]]:
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
            {out: rule.dependencies for out, rule in self._rules.items()}
        ).static_order()

    def graph_for(self, da: DataArray, targets: set[str]) -> Graph:
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
        if _is_in_coords(out_name, da):
            return FetchRule(
                (out_name,), da.coords, da.bins.coords if da.is_binned else {}
            )
        try:
            return self._rules[out_name]
        except KeyError:
            raise KeyError(
                f"Coordinate '{out_name}' does not exist in the input data "
                "and no rule has been provided to compute it."
            ) from None

    def show(self, size: str | None = None, simplified: bool = False) -> Digraph:
        dot = make_graphviz_digraph(strict=True)
        dot.attr('node', shape='box', height='0.1')
        dot.attr(size=size)
        for output, rule in self._rules.items():
            if isinstance(rule, RenameRule):
                dot.edge(rule.dependencies[0], output, style='dashed')
            elif isinstance(rule, ComputeRule):
                if not simplified:
                    # Get a unique name for every node,
                    # works because str contains address of func.
                    name = str(rule)
                    label = f'{rule.func_name}(...)'
                    dot.node(
                        name,
                        label=label,
                        shape='plain',
                        style='filled',
                        color='lightgrey',
                    )
                    dot.edge(name, output)
                else:
                    name = output
                for arg in rule.dependencies:
                    dot.edge(arg, name)
        return dot


def rule_sequence(rules: Graph) -> list[Rule]:
    already_used = set()
    result = []
    for rule in (
        r for n in rules.nodes_topologically() if (r := rules[n]) not in already_used
    ):
        already_used.add(rule)
        result.append(rule)
    return result


def _make_rule(products: tuple[str, ...], producer: str | Kernel) -> Rule:
    if isinstance(producer, str):
        return RenameRule(products, producer)
    return ComputeRule(products, producer)


def _convert_to_rule_graph(graph: GraphDict) -> dict[str, Rule]:
    rule_graph = {}
    for products, producer in graph.items():
        products = (products,) if isinstance(products, str) else tuple(products)
        rule = _make_rule(products, producer)
        for product in products:
            if product in rule_graph:
                raise ValueError(
                    f'Duplicate output name defined in conversion graph: {product}'
                )
            rule_graph[product] = rule
    return rule_graph


def _is_in_coords(name: str, da: DataArray) -> bool:
    return name in da.coords or (da.is_binned and name in da.bins.coords)
