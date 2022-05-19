# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ..model import Node


class WidgetNode(Node):

    def __init__(self, widget, *args, **kwargs):
        super().__init__(*args, **kwargs)
        widget.observe(self.notify_children, names="value")
