# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import DataArray

# class Pipeline(list):
#     """
#     A Pipeline is a list of filters that will be applied to the data
#     """
#     def run(self, model: DataArray):
#         out = model
#         for item in self:
#             out = item(out)
#         return out


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
