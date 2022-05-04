# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


class Plot:

    def __init__(self):
        self._models = {}

    def add_model(self, key, model):
        self._models[key] = model

    def render(self):
        for model in self._models.values():
            model.notify_from_dependents("root")

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
        for model in self._models.values():
            views += model.get_all_views()
        return ipw.VBox([view._to_widget() for view in set(views)])
