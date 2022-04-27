# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)


class NotificationHandler:

    def __init__(self):
        self._views = []

    def add_view(self, view):
        self._views.append(view)

    def notify_change(self, change):
        for view in self._views:
            view.notify(change)
