# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .widgets.widget import WidgetView

from typing import Tuple, Iterable
from functools import partial


class Node:

    def __init__(self, func, name=None, views=None):
        self.name = name
        self.parent_name = None
        self.dependency = None
        self.func = func
        self.views = list(views or [])

    def send_notification(self, message):
        for view in self.views:
            view.notify({
                "node_name": self.name,
                "parent_name": self.parent_name,
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


class Model:

    def __init__(self, da):
        self._name = da.name
        self._nodes = {}
        self["root"] = Node(name='root', func=lambda: da)

    def __getitem__(self, name: str) -> Node:
        return self._nodes[name]

    def __setitem__(self, name: str, node: Node):
        self._nodes[name] = node
        node.parent_name = self._name
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

    def insert(self, name: str, node: Node, after: str):
        assert after in self.keys()
        self[name] = node
        for child in self._children_of(after):
            child.dependency = node
        node.dependency = self[after]

    def add(self, name: str, node: Node, after: str):
        assert after in self.keys()
        self[name] = node
        node.dependency = self[after]

    def notify_from_dependents(self, node: str):
        depth_first_stack = [node]
        while depth_first_stack:
            node = depth_first_stack.pop()
            self[node].send_notification('dummy message')
            for child in self._children_of(node):
                depth_first_stack.append(child.name)

    def add_view(self, key, view):
        self[key].add_view(view)
        view.add_model_node(self[key])
        if isinstance(view, WidgetView):
            view.add_notification(partial(self.notify_from_dependents, node=key))

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
        raise RuntimeError('Failed to import `graphviz`, please install `graphviz` if '
                           'using `pip`, or `python-graphviz` if using `conda`.')
    return Digraph(*args, **kwargs)
