# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .controller import PlotController


class PlotController1d(PlotController):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def update_line_color(self, line_id=None, color=None):
        """
        Get a message from the panel to change the color of a given line.
        Forward the message to the view.
        """
        self.view.update_line_color(line_id=line_id, color=color)

    def connect_panel(self):
        self.panel.connect({
            "keep_line": self.keep_line,
            "remove_line": self.remove_line,
            "update_line_color": self.update_line_color
            })
