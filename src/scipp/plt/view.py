# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


class View:

    def __init__(self):
        self._model_nodes = {}

    def add_model_node(self, node):
        if node.parent_name not in self._model_nodes:
            self._model_nodes[node.parent_name] = {}
        self._model_nodes[node.parent_name][node.name] = node

    def notify(self, _):
        return

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()
