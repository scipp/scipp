# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from abc import ABC, abstractmethod


class View(ABC):

    def __init__(self):
        self._graph_nodes = {}

    def add_graph_node(self, node):
        self._graph_nodes[node.id] = node

    @abstractmethod
    def notify_view(self, _):
        return

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()
