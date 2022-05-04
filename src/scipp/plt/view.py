# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .figure import Figure
from .toolbar import Toolbar

from functools import partial
import ipywidgets as ipw


class View:

    def __init__(self):
        self._model_nodes = {}

    def add_model_node(self, node):
        if node.parent_name not in self._model_nodes:
            self._model_nodes[node.parent_name] = {}
        self._model_nodes[node.parent_name][node.name] = node
        # # self._model_nodes[key] = node
        # self._base_func = node.func
        # self._update_node_func(node)

    def notify(self, _):
        return

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()
