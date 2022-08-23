# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ...utils.graph import make_graphviz_digraph
from html import escape
from itertools import chain
from functools import partial
import uuid


def _add_graph_edges(dot, node, inventory, hide_views):
    name = node.id
    inventory.append(name)
    dot.node(name, label=escape(str(node.func)))
    for child in node.children:
        key = child.id
        if key not in inventory:
            dot.edge(name, key)
            _add_graph_edges(dot, child, inventory, hide_views)
    for parent in node.parents + list(node.kwparents.values()):
        key = parent.id
        if key not in inventory:
            dot.edge(key, name)
            _add_graph_edges(dot, parent, inventory, hide_views)
    if not hide_views:
        for view in node.views:
            key = view.id
            dot.node(key,
                     label=view.__class__.__name__,
                     shape='ellipse',
                     style='filled',
                     color='lightgrey')
            dot.edge(name, key)


def show_graph(node, size=None, hide_views=False):
    dot = make_graphviz_digraph(strict=True)
    dot.attr('node', shape='box', height='0.1')
    dot.attr(size=size)
    inventory = []
    _add_graph_edges(dot, node, inventory, hide_views)
    return dot


class Node:

    def __init__(self, func, *parents, **kwparents):
        if not callable(func):
            raise ValueError("A node can only be created using a callable func.")
        self.func = func
        self.id = str(uuid.uuid1())
        self.children = []
        self.views = []
        self.parents = list(parents)
        self.kwparents = dict(kwparents)
        for parent in chain(self.parents, self.kwparents.values()):
            parent.add_child(self)

    def request_data(self):
        args = (parent.request_data() for parent in self.parents)
        kwargs = {key: parent.request_data() for key, parent in self.kwparents.items()}
        return self.func(*args, **kwargs)

    def add_child(self, child):
        self.children.append(child)

    def add_view(self, view):
        self.views.append(view)

    def notify_children(self, message):
        self.notify_views(message)
        for child in self.children:
            child.notify_children(message)

    def notify_views(self, message):
        for view in self.views:
            view.notify_view({"node_id": self.id, "message": message})


def node(func, *args, **kwargs):
    partialized = partial(func, *args, **kwargs)

    def make_node(*args, **kwargs):
        return Node(partialized, *args, **kwargs)

    return make_node


def input_node(obj):
    return Node(lambda: obj)
