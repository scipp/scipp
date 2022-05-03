# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .filters import Filter

from functools import partial
import ipywidgets as ipw


class WidgetCollection(list):

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self) -> ipw.Widget:
        """
        Gather all widgets in a single container box.
        """
        return ipw.VBox([child._to_widget() for child in self])


class WidgetFilter(Filter):

    def __init__(self, func, widgets):
        super().__init__(func=func)
        self._base_func = func  # func taking data array, dim, and int
        self._widgets = widgets
        for widget in self._widgets.values():
            widget.observe(self._changed, names="value")
        self._update_func()

    @property
    def values(self):
        return {key: widget.value for key, widget in self._widgets.items()}

    def _update_func(self):
        self._func = partial(self._base_func, **self.values)

    def _changed(self, change):
        self._update_func()
        super()._changed()

    def _ipython_display_(self):
        """
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        """
        widgets = []
        for widget in self._widgets.values():
            widgets.append(
                widget._to_widget() if hasattr(widget, "_to_widget") else widget)
        return ipw.Box(widgets)
