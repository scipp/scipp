# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from abc import ABC, abstractmethod


class View(ABC):

    def __init__(self):
        self._model_nodes = {}
        self._notifications = []

    def add_model_node(self, node):
        self._model_nodes.setdefault(node.parent_name, {})[node.name] = node

    def add_notification(self, notification):
        self._notifications.append(notification)

    @abstractmethod
    def notify_view(self, _):
        return

    def notify_models(self):
        for notification in self._notifications:
            notification()

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()
