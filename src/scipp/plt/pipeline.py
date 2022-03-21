# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import DataArray


class Pipeline(list):
    def run(self, model: DataArray):
        out = model
        for step in self:
            out = step(out)
        return out


class Step:
    def __init__(self, func):
        self._func = func
        self._callbacks = []

    def __call__(self, data_array):
        return self._func(data_array)

    def _changed(self):
        for callback in self._callbacks:
            callback()  # callback will call Step.__call__

    def register_callback(self, callback):
        self._callbacks.append(callback)
