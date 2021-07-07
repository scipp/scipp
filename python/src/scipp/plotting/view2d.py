# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .view import PlotView
from .._variable import zeros
import numpy as np
from matplotlib.collections import PathCollection


class PlotView2d(PlotView):
    """
    View object for 2 dimensional plots. Contains a `PlotFigure2d`.

    The difference between `PlotView2d` and `PlotFigure2d` is that
    `PlotView2d` also handles the communications with the `PlotController` that
    are to do with the `PlotProfile` plot displayed below the `PlotFigure2d`.

    In addition, `PlotView2d` provides a dynamic image resampling for large
    input data.
    """
    def __init__(self, figure, formatters):
        super().__init__(figure=figure, formatters=formatters)
        self._axes = ['y', 'x']
        self._marker_index = []
        self._marks_scatter = None
        self._lim_updated = False
        self.current_lims = {}
        self.global_lims = {}

        for event in ['xlim_changed', 'ylim_changed']:
            self.figure.ax.callbacks.connect(event, self._lims_changed)

    def _make_data(self, new_values, mask_info):
        if not self.global_lims:
            for axis, dim in zip(['y', 'x'], new_values.dims):
                xmin = new_values.coords[dim].values[0]
                xmax = new_values.coords[dim].values[-1]
                self.global_lims[axis] = [xmin, xmax]
                self.current_lims[axis] = [xmin, xmax]
        values = new_values.values
        slice_values = {
            "values":
            values,
            "extent":
            np.array([self.current_lims['x'],
                      self.current_lims['y']]).flatten()
        }
        mask_info = next(iter(mask_info.values()))
        if len(mask_info) > 0:
            # Use automatic broadcasting in Scipp variables
            msk = zeros(sizes=new_values.sizes, dtype='int32')
            for m, val in mask_info.items():
                if val:
                    msk += new_values.masks[m].astype(msk.dtype)
            slice_values["masks"] = msk.values
        return slice_values

    def _lims_changed(self, *args):
        """
        Update limits and resample the image according to new viewport.

        When we use the zoom tool, the event listener on the displayed axes
        limits detects two separate events: one for the x axis and another for
        the y axis. We use a small locking mechanism here to trigger only a
        single resampling update by waiting for the y limits to also change.
        """
        if not self.global_lims:
            return
        if not self._lim_updated:
            self._lim_updated = True
            return
        self._lim_updated = False

        # Make sure we don't overrun the original array bounds
        xylims = {
            "x": np.clip(self.figure.ax.get_xlim(),
                         *sorted(self.global_lims["x"])),
            "y": np.clip(self.figure.ax.get_ylim(),
                         *sorted(self.global_lims["y"]))
        }

        dx = np.abs(self.current_lims["x"][1] - self.current_lims["x"][0])
        dy = np.abs(self.current_lims["y"][1] - self.current_lims["y"][0])
        diffx = np.abs(self.current_lims["x"] - xylims["x"]) / dx
        diffy = np.abs(self.current_lims["y"] - xylims["y"]) / dy
        diff = diffx.sum() + diffy.sum()

        # Only resample image if the changes in axes limits are large enough to
        # avoid too many updates while panning.
        if diff > 0.1:
            self.current_lims = xylims
            limits = {}
            for dim, ax in zip(self._data.dims, "yx"):
                low, high = xylims[ax]
                unit = self._data.coords[dim].unit
                limits[dim] = [low * unit, high * unit]
            self.controller.update_data(slices=limits)

        # If we are zooming, rescale to data?
        if self.figure.rescale_on_zoom():
            self.controller.rescale_to_data()

    def _update_axes(self):
        """
        Update the current and global axes limits, before updating the figure
        axes.
        """
        self.current_lims = {}
        self.global_lims = {}
        super()._update_axes()
        self.clear_marks()

    def clear_marks(self):
        """
        Reset all scatter markers when a profile is reset.
        """
        if self._marks_scatter is not None:
            self._marks_scatter = None
            self.figure.ax.collections = []
            self.figure.draw()

    def _do_handle_pick(self, event):
        """
        Return the index of the picked scatter point, None if something else
        is picked.
        """
        if isinstance(event.artist, PathCollection):
            return self._marker_index[event.ind[0]]

    def _do_mark(self, index, color, x, y):
        """
        Add a marker (colored scatter point).
        """
        if self._marks_scatter is None:
            self._marks_scatter = self.figure.ax.scatter([x], [y],
                                                         c=[color],
                                                         edgecolors="w",
                                                         picker=5,
                                                         zorder=10)
        else:
            new_offsets = np.concatenate(
                (self._marks_scatter.get_offsets(), [[x, y]]), axis=0)
            new_colors = np.concatenate(
                (self._marks_scatter.get_facecolors(), [color]), axis=0)
            self._marks_scatter.set_offsets(new_offsets)
            self._marks_scatter.set_facecolors(new_colors)
        self._marker_index.append(index)
        self.figure.draw()

    def remove_mark(self, index):
        """
        Remove a marker (scatter point).
        """
        i = self._marker_index.index(index)
        xy = np.delete(self._marks_scatter.get_offsets(), i, axis=0)
        c = np.delete(self._marks_scatter.get_facecolors(), i, axis=0)
        self._marks_scatter.set_offsets(xy)
        self._marks_scatter.set_facecolors(c)
        self._marker_index.remove(index)
        self.figure.draw()
