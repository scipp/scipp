# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from functools import partial
import ipywidgets as ipw


class Step:
    def __init__(self, func):
        self._func = func
        #self.func = func
        #self.widget = widget
        self._callbacks = []

    def __call__(self, data_array):
        return self._func(data_array)

    #def values(self):
    #    return self.widget.values()

    def register_callback(self, callback):
        self._callbacks.append(callback)
        #self.widget.set_callback(func)


class WidgetStep(Step):
    def __init__(self, func, widgets):
        super().__init__(func=func)
        self._base_func = func  # func taking data array, dim, and int
        self._widgets = widgets
        # connect(widget.event, self._changed)
        for widget in self._widgets.values():
            widget.observe(self._changed, names="value")
        self._update_func()

    @property
    def values(self):
        return {key: widget.value for key, widget in self._widgets.items()}

    #def _update_func(self):
    #    #self._func = partial(value=widget.value)
    #    self._func.value = widget.value

    def _update_func(self):
        self._func = partial(self._base_func, **self.values)

    def _changed(self, change):
        # self._func = partial(self._base_func, change["new"])
        self._update_func()
        #self_update_func()
        for callback in self._callbacks:
            callback()  # callback will call Step.__call__

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
