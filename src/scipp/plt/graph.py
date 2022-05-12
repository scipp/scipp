# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from typing import Tuple, Iterable
from functools import partial


class Node:

    def __init__(self, func, name=None, views=None):
        self.name = name
        self.graph_name = None
        self.dependency = None
        self.func = func
        self.views = list(views or [])

    def notify_views(self, message):
        for view in self.views:
            view.notify_view({
                "node_name": self.name,
                "graph_name": self.graph_name,
                "message": message
            })

    def request_data(self):
        pipeline = []
        node = self
        while node.dependency is not None:
            pipeline.append(node.func)
            node = node.dependency
        else:
            pipeline.append(node.func)
        pipeline = pipeline[::-1]
        data = pipeline[0]()
        for step in pipeline[1:]:
            data = step(data)
        return data

    def add_view(self, view):
        self.views.append(view)


class Graph:

    def __init__(self, da):
        self._name = da.name
        self._nodes = {}
        root_node = Node(func=lambda: da)
        self[""] = root_node

    def __getitem__(self, name: str) -> Node:
        return self._nodes[name]

    def __setitem__(self, name: str, node: Node):
        self._nodes[name] = node
        node.graph_name = self._name
        node.name = name

    def items(self) -> Iterable[Tuple[str, Node]]:
        yield from self._nodes.items()

    def _children_of(self, name: str) -> Iterable[str]:
        for node in self.nodes():
            if node.dependency is not None and name == node.dependency.name:
                yield node

    def keys(self) -> Iterable[str]:
        yield from self._nodes.keys()

    def nodes(self) -> Iterable[str]:
        yield from self._nodes.values()

    def _validate_names(self, name, after):
        assert after in self.keys()
        assert name not in self.keys()

    def insert(self, name: str, node: Node, after: str):
        """
        Insert a node in the graph, possibly rearranging the graph so that the
        children of the `after` node become the children of the new node.
        """
        self._validate_names(name=name, after=after)
        self[name] = node
        for child in self._children_of(after):
            child.dependency = node
        node.dependency = self[after]

    def add(self, name: str, node: Node, after: str):
        """
        Add a node in the graph. As opposed to `insert`, this does not rearrange
        the graph but simply makes a new branch from the `after` node.
        """
        self._validate_names(name=name, after=after)
        self[name] = node
        node.dependency = self[after]

    def notify_from_dependents(self, name: str):
        depth_first_stack = [name]
        while depth_first_stack:
            name = depth_first_stack.pop()
            self[name].notify_views('dummy message')
            for child in self._children_of(name):
                depth_first_stack.append(child.name)

    def add_view(self, node, view):
        node.add_view(view)
        view.add_graph_node(node)
        view.add_notification(partial(self.notify_from_dependents, name=node.name))

    @property
    def root(self) -> Node:
        for node in self.nodes():
            if node.dependency is None:
                return node

    @property
    def end(self) -> Node:
        ends = []
        for node in self.nodes():
            if len(tuple(self._children_of(node.name))) == 0:
                ends.append(node)
        if len(ends) != 1:
            raise RuntimeError(f'No unique end node: {ends}')
        return ends[0]

    def show(self, size=None):
        dot = _make_graphviz_digraph(strict=True)
        dot.attr('node', shape='box', height='0.1')
        dot.attr(size=size)
        for name, node in self.items():
            if node.dependency is not None:
                dot.edge(node.dependency.name, name)
            for view in node.views:
                key = str(view)
                dot.node(key, shape='ellipse', style='filled', color='lightgrey')
                dot.edge(name, key)

        return dot

    def get_all_views(self):
        views = []
        for node in self.nodes():
            for view in node.views:
                views.append(view)
        return views


def _make_graphviz_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError:
        raise RuntimeError(
            "Failed to import `graphviz`. "
            "Use `pip install graphviz` (requires installed `graphviz` executable) or "
            "`conda install -c conda-forge python-graphviz`.")
    return Digraph(*args, **kwargs)
