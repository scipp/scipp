# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


class Step:
    def __init__(self, func, widget):
        self.func = func
        self.widget = widget

    def values(self):
        return self.widget.values()

    def register_callback(self, func):
        self.widget.set_callback(func)
