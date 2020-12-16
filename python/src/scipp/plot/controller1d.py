# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .controller import PlotController


class PlotController1d(PlotController):
    """
    Controller class for 1d plots.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def update_line_color(self, line_id=None, color=None):
        """
        Get a message from the panel to change the color of a given line.
        Forward the message to the view.
        """
        self.view.update_line_color(line_id=line_id, color=color)

    def connect_panel(self):
        """
        Establish connection to the panel interface.
        """
        self.panel.connect({
            "keep_line": self.keep_line,
            "remove_line": self.remove_line,
            "update_line_color": self.update_line_color
        })

    def toggle_xaxis_scale(self, owner):
        """
        Toggle x-axis scale from toolbar button signal.
        """
        super().toggle_xaxis_scale(owner, normalize=True)
