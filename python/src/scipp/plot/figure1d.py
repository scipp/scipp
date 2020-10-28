# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .figure import PlotFigure
from .tools import get_line_param
import numpy as np
import copy as cp
import warnings


class PlotFigure1d(PlotFigure):
    """
    Class for 1 dimensional plots. This is used by both the `Plotview1d` for
    normal 1d plots, and the `PlotProfile`.

    `PlotFigure1d` can "keep" the currently displayed line, or "remove" a
    previously saved line.
    """
    def __init__(self,
                 errorbars=None,
                 ax=None,
                 mpl_line_params=None,
                 title=None,
                 unit=None,
                 norm=None,
                 grid=False,
                 masks=None,
                 figsize=None,
                 picker=False,
                 legend={"show": True},
                 padding=None):

        super().__init__(ax=ax, figsize=figsize, title=title, padding=padding)

        # Matplotlib line containers
        self.data_lines = {}
        self.mask_lines = {}
        self.error_lines = {}

        self.errorbars = errorbars
        self.masks = masks
        self.picker = picker
        self.unit = unit
        self.norm = norm
        self.legend = legend
        if "loc" not in self.legend:
            self.legend["loc"] = 0

        self.grid = grid

        # Save the line parameters (color, linewidth...)
        self.mpl_line_params = mpl_line_params

        for name in self.masks:
            if self.masks[name]["color"] is None:
                self.masks[name]["color"] = "k"

    def update_axes(self, axparams=None, clear=True, legend_labels=True):
        """
        Wipe the figure and start over when the dimension to be displayed along
        the horizontal axis is changed.
        """
        xparams = axparams["x"]
        print(xparams)

        if self.own_axes:
            self.ax.clear()

        if self.mpl_line_params is None:
            self.mpl_line_params = {
                "color": {},
                "marker": {},
                "linestyle": {},
                "linewidth": {}
            }
            for i, name in enumerate(xparams["hist"]):
                self.mpl_line_params["color"][name] = get_line_param(
                    "color", i)
                self.mpl_line_params["marker"][name] = get_line_param(
                    "marker", i)
                self.mpl_line_params["linestyle"][name] = get_line_param(
                    "linestyle", i)
                self.mpl_line_params["linewidth"][name] = get_line_param(
                    "linewidth", i)

        self.ax.set_xscale(xparams["scale"])
        self.ax.set_yscale("log" if self.norm == "log" else "linear")
        self.ax.set_ylabel(self.unit)

        if self.grid:
            self.ax.grid()

        deltax = 0.05 * (xparams["lims"][1] - xparams["lims"][0])
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self.ax.set_xlim(
                [xparams["lims"][0] - deltax, xparams["lims"][1] + deltax])

        self.ax.set_xlabel(xparams["label"])

        self.ax.xaxis.set_major_locator(
            self.axlocator[xparams["dim"]][xparams["scale"]])
        self.ax.xaxis.set_major_formatter(
            self.axformatter[xparams["dim"]][xparams["scale"]])

        for name, hist in xparams["hist"].items():

            label = None
            if legend_labels:
                label = name if len(name) > 0 else " "

            self.mask_lines[name] = {}

            if hist:
                [self.data_lines[name]] = self.ax.step(
                    [1, 2], [1, 2],
                    label=label,
                    zorder=10,
                    picker=self.picker,
                    **{
                        key: self.mpl_line_params[key][name]
                        for key in ["color", "linewidth"]
                    })
                for m in self.masks[name]["names"]:
                    [self.mask_lines[name][m]] = self.ax.step(
                        [1, 2], [1, 2],
                        linewidth=self.mpl_line_params["linewidth"][name] *
                        3.0,
                        color=self.masks[name]["color"],
                        zorder=9)
                    # Abuse a mostly unused property `gid` of Line2D to
                    # identify the line as a mask. We set gid to `onaxes`.
                    # This is used by the profile viewer in the 2D plotter
                    # to know whether to show the mask or not, depending on
                    # whether the cursor is hovering over the 2D image or
                    # not.
                    self.mask_lines[name][m].set_gid("onaxes")
            else:
                [self.data_lines[name]] = self.ax.plot(
                    [1, 2], [1, 2],
                    label=label,
                    zorder=10,
                    picker=self.picker,
                    **{
                        key: self.mpl_line_params[key][name]
                        for key in self.mpl_line_params.keys()
                    })
                for m in self.masks[name]["names"]:
                    [self.mask_lines[name][m]] = self.ax.plot(
                        [1, 2], [1, 2],
                        zorder=11,
                        mec=self.masks[name]["color"],
                        mfc="None",
                        mew=3.0,
                        linestyle="none",
                        marker=self.mpl_line_params["marker"][name])
                    self.mask_lines[name][m].set_gid("onaxes")

            if self.picker:
                self.data_lines[name].set_pickradius(5.0)
            self.data_lines[name].set_url(name)

            # Add error bars
            if self.errorbars[name]:
                self.error_lines[name] = self.ax.errorbar(
                    [1, 2], [1, 2],
                    yerr=[1, 1],
                    color=self.mpl_line_params["color"][name],
                    zorder=10,
                    fmt="none")

        if self.show_legend():
            self.ax.legend(loc=self.legend["loc"])

    def update_data(self, new_values, info):
        """
        Update the x and y positions of the data points when a new data slice
        is received for display.
        """
        for name, vals in new_values.items():

            self.data_lines[name].set_data(vals["values"]["x"],
                                           vals["values"]["y"])
            lab = info["slice_label"] if len(info["slice_label"]) > 0 else name
            self.data_lines[name].set_label(lab)

            for m in vals["masks"]:
                self.mask_lines[name][m].set_data(vals["values"]["x"],
                                                  vals["masks"][m])

            if self.errorbars[name]:
                coll = self.error_lines[name].get_children()[0]
                coll.set_segments(
                    self._change_segments_y(vals["variances"]["x"],
                                            vals["variances"]["y"],
                                            vals["variances"]["e"]))

        self.draw()

    def keep_line(self, name, color, line_id):
        """
        Duplicate the current main line and give it an arbitrary color.
        Triggered by a `PlotPanel1d` keep button or a `keep_profile` event.
        """
        # The main line
        self.ax.lines.append(cp.copy(self.data_lines[name]))
        self.ax.lines[-1].set_url(line_id)
        self.ax.lines[-1].set_zorder(2)
        if self.ax.lines[-1].get_marker() == "None":
            self.ax.lines[-1].set_color(color)
        else:
            self.ax.lines[-1].set_markerfacecolor(color)
            self.ax.lines[-1].set_markeredgecolor("None")

        # The masks
        for m in self.mask_lines[name]:
            self.ax.lines.append(cp.copy(self.mask_lines[name][m]))
            self.ax.lines[-1].set_url(line_id)
            self.ax.lines[-1].set_gid(m)
            self.ax.lines[-1].set_zorder(3)
            if self.ax.lines[-1].get_marker() != "None":
                self.ax.lines[-1].set_zorder(3)
            else:
                self.ax.lines[-1].set_zorder(1)

        if self.errorbars[name]:
            err = self.error_lines[name].get_children()
            self.ax.collections.append(cp.copy(err[0]))
            self.ax.collections[-1].set_color(color)
            self.ax.collections[-1].set_url(line_id)
            self.ax.collections[-1].set_zorder(2)

        if self.show_legend():
            self._reset_line_label(name)
            self.ax.legend(loc=self.legend["loc"])
        self.draw()

    def _reset_line_label(self, name):
        """
        When a line is saved, it is useful to see its parameters (location and
        thickness of slice that correspond to the line). The main line label
        is thus constantly updated by the `PlotModel` when a dimension slider
        is being moved. This can then easily be duplicated in `keep_line()`.
        Just before we show the legend, we need to reset the line
        name to its original name.
        """
        self.data_lines[name].set_label(name)

    def remove_line(self, name, line_id):
        """
        Remove a previously saved line.
        Triggered by a `PlotPanel1d` remove button or a `remove_profile` event.
        """
        lines = []
        for line in self.ax.lines:
            if line.get_url() != line_id:
                lines.append(line)
        collections = []
        for coll in self.ax.collections:
            if coll.get_url() != line_id:
                collections.append(coll)
        self.ax.lines = lines
        self.ax.collections = collections
        if self.show_legend():
            self._reset_line_label(name)
            self.ax.legend(loc=self.legend["loc"])
        self.draw()

    def update_line_color(self, line_id, color):
        """
        Change the line color when the `ColorPicker` in the `PlotPanel1d` is
        being used.
        """
        for line in self.ax.lines:
            if line.get_url() == line_id:
                if line.get_marker() == 'None':
                    line.set_color(color)
                else:
                    line.set_markerfacecolor(color)

        for coll in self.ax.collections:
            if coll.get_url() == line_id:
                coll.set_color(color)
        self.draw()

    def _change_segments_y(self, x, y, e):
        """
        Update the positions of the errorbars when `update_data` is called.
        """
        arr1 = np.repeat(x, 2)
        arr2 = np.array([y - e, y + e]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    def toggle_mask(self, mask_group, mask_name, value):
        """
        Show or hide a given mask.
        """
        if mask_group in self.mask_lines:
            msk = self.mask_lines[mask_group][mask_name]
            if msk.get_gid() == "onaxes":
                msk.set_visible(value)
        # Also toggle masks on additional lines created by keep button
        for line in self.ax.lines:
            if line.get_gid() == mask_name:
                line.set_visible(value)
        self.draw()

    def rescale_to_data(self, vmin=None, vmax=None):
        """
        Automatically rescale x and y axes to the contents of the plot.
        """
        self.ax.autoscale(True)
        self.ax.relim()
        self.ax.autoscale_view()
        self.draw()

    def show_legend(self):
        """
        Only display legend if there is least 1 line in the plot.
        """
        return self.legend["show"] and len(
            self.ax.get_legend_handles_labels()[1]) > 0
