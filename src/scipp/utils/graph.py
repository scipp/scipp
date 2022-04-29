# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet, Jan-Lukas Wynen

from graphlib import TopologicalSorter
from typing import Dict, Iterable, Protocol, Tuple


class Node(Protocol):

    @property
    def dependencies(self) -> Tuple[str]:
        ...


class Graph:

    def __init__(self, nodes: Dict[str, Node]):
        self._nodes: Dict[str, Node] = nodes

    def __getitem__(self, name: str) -> Node:
        return self._nodes[name]

    def __setitem__(self, name: str, node: Node):
        self._nodes[name] = node

    def items(self) -> Iterable[Tuple[str, Node]]:
        yield from self._nodes.items()

    def parents_of(self, name: str) -> Iterable[str]:
        try:
            yield from self._nodes[name].dependencies
        except KeyError:
            # Input nodes have no parents but are not represented in the
            # graph unless the corresponding FetchRules have been added.
            return

    def children_of(self, name: str) -> Iterable[str]:
        for candidate, node in self.items():
            if name in node.dependencies:
                yield candidate

    def nodes(self) -> Iterable[str]:
        yield from self._nodes.keys()

    def nodes_topologically(self) -> Iterable[str]:
        yield from TopologicalSorter(
            {out: node.dependencies
             for out, node in self.items()}).static_order()
