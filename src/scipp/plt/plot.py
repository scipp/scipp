# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


class Plot:

    def __init__(self):
        self._graphs = {}

    def add_graph(self, key, graph):
        self._graphs[key] = graph

    def render(self):
        for graph in self._graphs.values():
            graph.notify_from_dependents(graph.root.name)

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        self.render()
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        """
        import ipywidgets as ipw
        views = []
        for graph in self._graphs.values():
            views += graph.get_all_views()
        return ipw.VBox([view._to_widget() for view in set(views)])
