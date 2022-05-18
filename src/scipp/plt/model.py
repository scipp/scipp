# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import uuid


def _make_graphviz_digraph(*args, **kwargs):
    try:
        from graphviz import Digraph
    except ImportError:
        raise RuntimeError(
            "Failed to import `graphviz`. "
            "Use `pip install graphviz` (requires installed `graphviz` executable) or "
            "`conda install -c conda-forge python-graphviz`.")
    return Digraph(*args, **kwargs)


def _add_graph_edges(dot, node, inventory, hide_views):
    name = str(node.func)
    inventory.append(name)
    for child in node.children:
        key = str(child.func)
        if key not in inventory:
            dot.edge(name, key)
            _add_graph_edges(dot, child, inventory, hide_views)
    for parent in node.parents.values():
        key = str(parent.func)
        if key not in inventory:
            dot.edge(key, name)
            _add_graph_edges(dot, parent, inventory, hide_views)
    if not hide_views:
        for view in node.views:
            key = str(view)
            dot.node(key, shape='ellipse', style='filled', color='lightgrey')
            dot.edge(name, key)


def show_graph(node, size=None, hide_views=False):
    dot = _make_graphviz_digraph(strict=True)
    dot.attr('node', shape='box', height='0.1')
    dot.attr(size=size)
    inventory = []
    _add_graph_edges(dot, node, inventory, hide_views)
    return dot


class Node:

    def __init__(self, func, parents=None, views=None):
        self.id = str(uuid.uuid1())
        self.children = []
        self.func = func
        self.parents = {}
        self.views = list(views or [])
        if parents is not None:
            for key, parent in parents.items():
                parent.add_child(self)
                self.parents[key] = parent

    def request_data(self):
        inputs = {key: parent.request_data() for key, parent in self.parents.items()}
        return self.func(**inputs)

    def add_child(self, child):
        self.children.append(child)

    def add_view(self, view):
        self.views.append(view)
        view.add_graph_node(self)

    def notify_children(self, message):
        self.notify_views(message)
        for child in self.children:
            child.notify_children(message)

    def notify_views(self, message):
        for view in self.views:
            view.notify_view({"node_id": self.id, "message": message})
