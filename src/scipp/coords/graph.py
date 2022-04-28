# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock, Jan-Lukas Wynen

from __future__ import annotations

from typing import Callable, Dict, List, Set, Tuple, Union

from ..core import DataArray
from ..utils.graph import Graph
from .rule import ComputeRule, FetchRule, RenameRule, Rule

GraphDict = Dict[Union[str, Tuple[str, ...]], Union[str, Callable]]


class RuleGraph(Graph):
    def __init__(self, graph: Union[GraphDict, Dict[str, Rule]]):
        if not graph:
            rules = {}
        elif isinstance(next(iter(graph.values())), Rule):
            rules = graph
        else:
            rules = _convert_to_rule_graph(graph)
        super().__init__(rules)

    def graph_for(self, da: DataArray, targets: Set[str]) -> RuleGraph:
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
        return RuleGraph(subgraph)

    def _rule_for(self, out_name: str, da: DataArray) -> Rule:
        if _is_in_meta_data(out_name, da):
            return FetchRule((out_name, ), da.meta, da.bins.meta if da.bins else {})
        try:
            return self[out_name]  # type: ignore
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


def rule_sequence(rules: RuleGraph) -> List[Rule]:
    already_used = set()
    result = []
    for rule in filter(lambda r: r not in already_used,
                       map(lambda n: rules[n], rules.nodes_topologically())):
        already_used.add(rule)
        result.append(rule)
    return result  # type: ignore


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
