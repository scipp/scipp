# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
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
        self._xstart = 1
        self._xend = 2

    def _to_widget(self):
        """
        When using the inline backend, the figure is rendered as an image, so
        if we want to hide the profile plot, we need to hide the entire widget
        and not just the canvas, as in toggle_view().
        """
        widg = super()._to_widget()
        if not self.is_widget():
            widg.layout.display = None if self.visible else 'none'
        return widg

    def update_axes(self, *args, **kwargs):
        """
        Upon axes update, we reset the slice area indicator.
        """
        super().update_axes(*args, legend_labels=False, **kwargs)
        # PlotFigure1d.update_axes clears self.ax
        self.slice_area = self.ax.axvspan(self._xstart,
                                          self._xend,
                                          alpha=0.5,
                                          color='lightgrey')

    def toggle_hover_visibility(self, value):
        """
        If the mouse moves off the image, we hide the profile. If it moves
        back onto the image, we show the profile.
        """
        for line in self._lines.values():
            line.data.set_visible(value)
            # Need to get the 3rd element of the errorbar container, which
            # contains the vertical errorbars, and then the first element of
            # that because it is a tuple itself.
            if line.error is not None:
                line.error[2][0].set_visible(value)
            for ml in line.masks.values():
                ml.set_visible(value)
                ml.set_gid("onaxes" if value else "offaxes")
        if value != self.current_visible_state:
            if value:
                self.rescale_to_data()
            self.current_visible_state = value

    def set_slice_area(self, xstart, xend):
        """
        When the data slice is updated (position or thickness), we update the
        slice area indicator.
        """
        self._xstart = xstart
        self._xend = xend
        if self.slice_area is not None:
            new_xy = np.array([[xstart, 0.0], [xstart, 1.0], [xend, 1.0], [xend, 0.0],
                               [xstart, 0.0]])
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
            self.toolbar.set_resampling_mode_display(False)
            self.toolbar.connect(controller=self)

    def toggle_dim_scale(self, dim):
        """
        Toggle x-axis scale from toolbar button signal.
        """
        def toggle(change):
            self.ax.set_xscale("log" if change['new'] else "linear")
            self.draw()

        return toggle

    def toggle_norm(self, owner):
        """
        Toggle y-axis scale from toolbar button signal.
        """
        self.ax.set_yscale("log" if owner.value else "linear")
        self.draw()
