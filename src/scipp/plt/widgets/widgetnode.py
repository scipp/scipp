# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ..model import Node


def widget_node(widget):
    n = Node(func=lambda: widget.value)
    widget.observe(n.notify_children, names="value")
    return n
