# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from functools import partial


class Filter:
    def __init__(self, func):
        self._func = func
        self._callbacks = []
        self._models = None

    def __call__(self, data_array):
        return self._func(data_array)

    def _changed(self):
        for callback in self._callbacks:
            callback()  # callback will call Filter.__call__

    def register_callback(self, callback):
        self._callbacks.append(callback)

    def register_models(self, models):
        self._models = models


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
        import ipywidgets as ipw
        widgets = []
        for widget in self._widgets.values():
            widgets.append(
                widget._to_widget() if hasattr(widget, "_to_widget") else widget)
        return ipw.Box(widgets)

    def notify(self, _):
        return
