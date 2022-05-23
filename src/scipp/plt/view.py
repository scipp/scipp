# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from abc import ABC, abstractmethod
import uuid


class View(ABC):

    def __init__(self, *nodes):
        self.id = str(uuid.uuid1())
        self._graph_nodes = {}
        for node in nodes:
            self._graph_nodes[node.id] = node
            node.add_view(self)

    @abstractmethod
    def notify_view(self, _):
        return

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()
