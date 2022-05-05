# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from abc import ABC, abstractmethod


class View(ABC):

    def __init__(self):
        self._model_nodes = {}

    def add_model_node(self, node):
        self._model_nodes.setdefault(node.parent_name, {})[node.name] = node

    @abstractmethod
    def notify(self, _):
        return

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()
