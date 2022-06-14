# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


def _maybe_to_widget(view):
    return view._to_widget() if hasattr(view, "_to_widget") else view


class Plot:

    def __init__(self, views):
        self.views = views

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        """
        import ipywidgets as ipw
        out = []
        for view in self.views:
            if isinstance(view, (list, tuple)):
                out.append(ipw.HBox([_maybe_to_widget(v) for v in view]))
            else:
                out.append(_maybe_to_widget(view))
        return ipw.VBox(out)
