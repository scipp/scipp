# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .figure2d import PlotFigure2d
from .view import PlotView
from .._utils import make_random_color
import numpy as np
from matplotlib.collections import PathCollection


class PlotView2d(PlotView):
    def __init__(self, *args, **kwargs):

        super().__init__(figure=PlotFigure2d(*args, **kwargs))

        self.xlim_updated = False
        self.ylim_updated = False
        self.current_lims = {"x": np.zeros(2), "y": np.zeros(2)}
        self.global_lims = {"x": np.zeros(2), "y": np.zeros(2)}

        # Connect changes in axes limits to resampling function
        self.figure.ax.callbacks.connect('xlim_changed',
                                         self.check_for_xlim_update)
        self.figure.ax.callbacks.connect('ylim_changed',
                                         self.check_for_ylim_update)

    def toggle_mask(self, change):
        self.figure.toggle_mask(change["owner"].mask_name, change["new"])

    def check_for_xlim_update(self, event_ax):
        self.xlim_updated = True
        if self.ylim_updated:
            self.update_bins_from_axes_limits()

    def check_for_ylim_update(self, event_ax):
        self.ylim_updated = True
        if self.xlim_updated:
            self.update_bins_from_axes_limits()

    def update_bins_from_axes_limits(self):
        """
        Update the axis limits and resample the image according to new viewport
        """
        self.xlim_updated = False
        self.ylim_updated = False
        xylims = {}

        # Make sure we don't overrun the original array bounds
        xylims["x"] = np.clip(self.figure.ax.get_xlim(),
                              *sorted(self.global_lims["x"]))
        xylims["y"] = np.clip(self.figure.ax.get_ylim(),
                              *sorted(self.global_lims["y"]))

        dx = np.abs(self.current_lims["x"][1] - self.current_lims["x"][0])
        dy = np.abs(self.current_lims["y"][1] - self.current_lims["y"][0])
        diffx = np.abs(self.current_lims["x"] - xylims["x"]) / dx
        diffy = np.abs(self.current_lims["y"] - xylims["y"]) / dy
        diff = diffx.sum() + diffy.sum()

        # Only resample image if the changes in axes limits are large enough to
        # avoid too many updates while panning.
        if diff > 0.1:
            self.current_lims = xylims
            self.interface["update_viewport"](xylims)

    def update_axes(self, axparams):

        self.current_lims['x'] = axparams["x"]["lims"]
        self.current_lims['y'] = axparams["y"]["lims"]
        self.global_lims["x"] = axparams["x"]["lims"]
        self.global_lims["y"] = axparams["y"]["lims"]

        self.figure.update_axes(axparams)
        self.reset_profile()

    def reset_profile(self):
        if self.profile_scatter is not None:
            self.profile_scatter = None
            self.figure.ax.collections = []
            self.figure.draw()

    def update_profile(self, event):
        if event.inaxes == self.figure.ax:
            xdata = event.xdata - self.current_lims["x"][0]
            ydata = event.ydata - self.current_lims["y"][0]
            self.interface["update_profile"](xdata, ydata)
            self.interface["toggle_hover_visibility"](True)
        else:
            self.interface["toggle_hover_visibility"](False)

    def keep_or_remove_profile(self, event):
        if isinstance(event.artist, PathCollection):
            self.remove_profile(event)
            # We need a profile lock to catch the second time the function is
            # called because the pick event is registed by both the scatter
            # points and the image
            self.profile_update_lock = True
        elif self.profile_update_lock:
            self.profile_update_lock = False
        else:
            self.keep_profile(event)
        self.figure.draw()

    def keep_profile(self, event):
        xdata = event.mouseevent.xdata
        ydata = event.mouseevent.ydata
        col = make_random_color(fmt='rgba')
        self.profile_counter += 1
        line_id = self.profile_counter
        self.profile_ids.append(line_id)
        if self.profile_scatter is None:
            self.profile_scatter = self.figure.ax.scatter([xdata], [ydata],
                                                          c=[col],
                                                          picker=5)
        else:
            new_offsets = np.concatenate(
                (self.profile_scatter.get_offsets(), [[xdata, ydata]]), axis=0)
            new_colors = np.concatenate(
                (self.profile_scatter.get_facecolors(), [col]), axis=0)
            self.profile_scatter.set_offsets(new_offsets)
            self.profile_scatter.set_facecolors(new_colors)

        self.interface["keep_line"](target="profile",
                                    color=col,
                                    line_id=line_id)

    def remove_profile(self, event):
        ind = event.ind[0]
        xy = np.delete(self.profile_scatter.get_offsets(), ind, axis=0)
        c = np.delete(self.profile_scatter.get_facecolors(), ind, axis=0)
        self.profile_scatter.set_offsets(xy)
        self.profile_scatter.set_facecolors(c)
        # Also remove the line from the 1d plot
        self.interface["remove_line"](target="profile",
                                      line_id=self.profile_ids[ind])
        self.profile_ids.pop(ind)
