# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
        super().toggle_xaxis_scale(owner)
        self.rescale_to_data()

    def toggle_yaxis_scale(self, owner):
        """
        Toggle x-axis scale from toolbar button signal.
        """
        super().toggle_yaxis_scale(owner)
        self.rescale_to_data()

    def refresh(self):
        """
        Do nothing for refresh in case of 1d plot.
        """
        return

    def rescale_to_data(self, button=None):
        """
        Automatically rescale the y axis (1D plot) or the colorbar (2D+3D
        plots) to the minimum and maximum value inside the currently displayed
        data slice.
        A small delta is used to add padding around the plotted points.
        """
        vmin, vmax = self.find_vmin_vmax(button=button)
        delta = 0.05 * (vmax - vmin)
        self.view.rescale_to_data(vmin - delta, vmax + delta)
