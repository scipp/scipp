# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .view import PlotView
from ..utils import make_random_color
from .. import ones
from .tools import vars_to_err
from .._scipp import core as sc
import numpy as np


class PlotView1d(PlotView):
    """
    View object for 1 dimensional plots. Contains a `PlotFigure1d`.

    The difference between `PlotView1d` and `PlotFigure1d` is that `PlotView1d`
    also handles the communications with the `PlotController` that are to do
    with the `PlotProfile` plot displayed below the `PlotFigure1d`.

    """
    def __init__(self, figure, formatters):
        super().__init__(figure=figure, formatters=formatters)
        self._axes = ['x']

    def _make_masks(self, array, mask_info, transpose=False):
        if not mask_info:
            return {}
        masks = {}
        data = array.data
        base_mask = ones(sizes=data.sizes, dtype=sc.dtype.int32)
        for m in mask_info:
            if m in array.masks:
                msk = base_mask * sc.Variable(dims=array.masks[m].dims,
                                              values=array.masks[m].values)
                masks[m] = msk.values
                if transpose:
                    masks[m] = sc.transpose(msk).values
            else:
                masks[m] = None
        return masks

    def _make_data(self, new_values, mask_info):
        out = {}
        for name, array in new_values.items():
            self._dim = array.dims[0]  # should be same for all items
            values = {"values": {}, "variances": {}, "masks": {}}
            values["values"]["x"] = array.meta[self._dim].values.ravel()
            values["values"]["y"] = array.values.ravel()
            if array.variances is not None:
                values["variances"]["e"] = vars_to_err(array.variances.ravel())
            values["masks"] = self._make_masks(array,
                                               mask_info=mask_info[name])
            out[name] = values
        return out

    def toggle_mask(self, change):
        """
        Forward mask toggling to the `figure`.
        """
        self.figure.toggle_mask(change["owner"].mask_group,
                                change["owner"].mask_name, change["new"])

    def keep_line(self, *args, **kwargs):
        """
        Forward keep line event to the `figure`.
        """
        self.figure.keep_line(*args, **kwargs)

    def remove_line(self, *args, **kwargs):
        """
        Forward remove line event to the `figure`.
        """
        self.figure.remove_line(*args, **kwargs)

    def update_line_color(self, *args, **kwargs):
        """
        Forward line color update to the `figure`.
        """
        self.figure.update_line_color(*args, **kwargs)

    def reset_profile(self):
        """
        Remove all vertical lines (=profile location markers).
        """
        new_lines = []
        for line in self.figure.ax.lines:
            if not (line.get_url() == "axvline"):
                new_lines.append(line)
        self.figure.ax.lines = new_lines
        self.figure.draw()

    def update_profile(self, event):
        """
        If mouse is hovering inside the axes, show and update profile.
        Otherwise, hide profile.

        TODO: optimize visibility update to that it only calls the function on
        a state change and not on every mouse movement.
        """
        # Plan:
        # want to use separate model for profile
        # => want to refactor `model.update_profile` so it is same as
        #    update_data
        # => move cursor finding code from model here
        # => need to store self.data from update_data
        # => new_values should be data array (or list of data array)
        if event.inaxes == self.figure.ax:
            #  Find closest point to cursor
            #  TODO: can we optimize this with new buckets?
            distance_to_cursor = np.abs(
                next(iter(self._data.values())).meta[self._dim].values -
                event.xdata)
            ind = int(np.argmin(distance_to_cursor))
            slices = {self._dim: ind}
            self.interface["update_profile"](slices)
            self.interface["toggle_hover_visibility"](True)
        else:
            self.interface["toggle_hover_visibility"](False)

    def keep_or_remove_profile(self, event):
        """
        Forward the keep or remove event to the correct function.
        """
        line_url = event.artist.get_url()
        if line_url == "axvline":
            self.remove_profile(event)
        else:
            self.keep_profile(event, line_url)
        self.figure.draw()

    def keep_profile(self, event, line_name):
        """
        Add a vertical line to mark the location of the saved profile.
        """
        # TODO: The names of the data variables are stored in the masks
        # information. This is not very clean.
        if line_name in self.figure.masks:
            xdata = event.mouseevent.xdata
            col = make_random_color(fmt='hex')
            self.profile_counter += 1
            line_id = self.profile_counter
            line = self.figure.ax.axvline(xdata, color=col, picker=True)
            line.set_pickradius(5.0)
            line.set_url("axvline")
            line.set_gid(line_id)
            self.interface["keep_line"](target="profile",
                                        name=line_name,
                                        color=col,
                                        line_id=line_id)

    def remove_profile(self, event):
        """
        Remove a vertical line corresponding to a saved profile.
        """
        new_lines = []
        gid = event.artist.get_gid()
        url = event.artist.get_url()
        for line in self.figure.ax.lines:
            if not ((line.get_gid() == gid) and (line.get_url() == url)):
                new_lines.append(line)
        self.figure.ax.lines = new_lines

        # Also remove the line from the 1d plot
        self.interface["remove_line"](target="profile", line_id=gid)
