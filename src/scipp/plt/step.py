# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from functools import partial


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
    def __init__(self, func, widget):
        super().__init__(func=func)
        self._base_func = func  # func taking data array, dim, and int
        self._widget = widget
        # connect(widget.event, self._changed)
        self._widget.observe(self._changed, names="value")
        self._set_func(value=self.value)

    @property
    def value(self):
        return self._widget.value

    #def _update_func(self):
    #    #self._func = partial(value=widget.value)
    #    self._func.value = widget.value

    def _set_func(self, value):
        self._func = partial(self._base_func, value)

    def _changed(self, change):
        # self._func = partial(self._base_func, change["new"])
        self._set_func(value=change["new"])
        #self_update_func()
        for callback in self._callbacks:
            callback()  # c will call Step.__call__

    def _ipython_display_(self):
        """
        """
        return self._widget._ipython_display_()

    def _to_widget(self):
        """
        """
        return self._widget._to_widget()
