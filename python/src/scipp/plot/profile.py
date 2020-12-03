# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .figure1d import PlotFigure1d
import numpy as np


class PlotProfile(PlotFigure1d):
    """
    Class for 1 dimensional profile plots, that are displayed as 1d slices into
    a higher dimensional data.

    This is using the `PlotFigure1d` extensively which can display 1d lines and
    keep or remove lines for comparison of spectra.
    In addition, it has a `slice_area` indicator to show the current range
    occupied by the slice that is displayed in the main subplot as a grey
    rectangle (using `axvspan`).

    Finally, it shows or hide the profile line depending on whether the mouse
    cursor is inside the main plot axes or not (`toggle_hover_visibility`).
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.current_visible_state = False
        self.slice_area = None
        self.visible = False

    def update_axes(self, *args, **kwargs):
        """
        Upon axes update, we reset the slice area indicator.
        """
        super().update_axes(*args, legend_labels=False, **kwargs)
        self.slice_area = self.ax.axvspan(1, 2, alpha=0.5, color='lightgrey')

    def _reset_line_label(self, name):
        """
        In the profile plot, we do not repeat the data name, as it is already
        displayed in the main subplot (as the title for 2d plots and as a
        legend entry for 1d plots).
        """
        self.data_lines[name].set_label(None)

    def toggle_hover_visibility(self, value):
        """
        If the mouse moves off the image, we hide the profile. If it moves
        back onto the image, we show the profile.
        """
        for name in self.data_lines:
            self.data_lines[name].set_visible(value)
        for name in self.error_lines:
            # Need to get the 3rd element of the errorbar container, which
            # contains the vertical errorbars, and then the first element of
            # that because it is a tuple itself.
            self.error_lines[name][2][0].set_visible(value)
        for name, mlines in self.mask_lines.items():
            for ml in mlines.values():
                ml.set_visible(value)
                ml.set_gid("onaxes" if value else "offaxes")
        if value != self.current_visible_state:
            if value:
                self.rescale_to_data()
            self.current_visible_state = value

    def update_slice_area(self, xstart, xend):
        """
        When the data slice is updated (position or thickness), we update the
        slice area indicator.
        """
        new_xy = np.array([[xstart, 0.0], [xstart, 1.0], [xend, 1.0],
                           [xend, 0.0], [xstart, 0.0]])
        self.slice_area.set_xy(new_xy)
        self.draw()

    def toggle_view(self, visible=True):
        """
        Show or hide profile view.
        """
        if self.is_widget():
            self.fig.canvas.layout.display = None if visible else 'none'
            self.toolbar.set_visible(visible)
        self.visible = visible

    def is_visible(self):
        """
        Get visible state of profile view.
        """
        return self.visible

    def connect(self):
        """
        For the profile, we connect the log buttons of the toolbar directly to
        callbacks local to `PlotProfile`, since all we need to do is toggle
        the scale on the matplotlib axes.
        """
        if self.toolbar is not None:
            self.toolbar.connect({
                "toggle_xaxis_scale": self.toggle_xaxis_scale,
                "toggle_norm": self.toggle_norm,
                "home_view": self.home_view,
                "pan_view": self.pan_view,
                "zoom_view": self.zoom_view,
                "save_view": self.save_view
            })

    def toggle_xaxis_scale(self, owner):
        """
        Toggle x-axis scale from toolbar button signal.
        """
        self.ax.set_xscale("log" if owner.value else "linear")
        self.draw()

    def toggle_norm(self, owner):
        """
        Toggle y-axis scale from toolbar button signal.
        """
        self.ax.set_yscale("log" if owner.value else "linear")
        self.draw()

    # def pan_view(self, owner):
    #     self.toolbar.pan_view()

    # def zoom_view(self, owner):
    #     self.toolbar.zoom_view()

    # def save_view(self, owner):
    #     self.toolbar.save_view()
