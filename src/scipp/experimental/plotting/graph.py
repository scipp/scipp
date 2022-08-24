# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ...utils.graph import make_graphviz_digraph
from html import escape


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
