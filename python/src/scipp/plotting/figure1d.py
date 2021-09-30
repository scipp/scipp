# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .figure import PlotFigure
from .toolbar import PlotToolbar1d
from .tools import get_line_param
import numpy as np
import copy as cp
import warnings


class PlotFigure1d(PlotFigure):
    """
    Class for 1 dimensional plots. This is used by both the `PlotView1d` for
    normal 1d plots, and the `PlotProfile`.

    `PlotFigure1d` can "keep" the currently displayed line, or "remove" a
    previously saved line.
    """
    def __init__(self,
                 ax=None,
                 mpl_line_params=None,
                 title=None,
                 norm=None,
                 grid=False,
                 mask_color=None,
                 figsize=None,
                 picker=False,
                 legend=None,
                 padding=None,
                 xlabel=None,
                 ylabel=None):

        super().__init__(ax=ax,
                         figsize=figsize,
                         title=title,
                         padding=padding,
                         xlabel=xlabel,
                         ylabel=ylabel,
                         toolbar=PlotToolbar1d,
                         grid=grid)

        self._lines = {}

        if legend is None:
            legend = {"show": True}
        elif isinstance(legend, bool):
            legend = {"show": legend}
        elif "show" not in legend:
            legend["show"] = True

        self._mask_color = mask_color if mask_color is not None else 'k'
        self.picker = picker
        self.norm = norm
        self.legend = legend
        if "loc" not in self.legend:
            self.legend["loc"] = 0

        self._mpl_line_params = mpl_line_params  # color, linewidth, ...

    def update_axes(self, scale, unit, legend_labels=True):
        """
        Wipe the figure and start over when the dimension to be displayed along
        the horizontal axis is changed.
        """
        scale = scale['x']
        self._legend_labels = legend_labels

        if self.own_axes:
            self._lines = {}
            title = self.ax.get_title()
            need_grid = self.ax.xaxis.get_gridlines()[0]._visible
            self.ax.clear()
            self.ax.set_title(title)
            if need_grid:
                self.ax.grid()

        self.ax.set_xscale(scale)
        self.ax.set_yscale("log" if self.norm == "log" else "linear")
        self.ax.set_ylabel(unit if self.ylabel is None else self.ylabel)

        self.ax.set_xlabel(
            self._formatters['x']['label'] if self.xlabel is None else self.xlabel)

        self.ax.xaxis.set_major_locator(self.axlocator['x'][scale])
        self.ax.xaxis.set_major_formatter(self.axformatter['x'][scale])

        if self.show_legend():
            self.ax.legend(loc=self.legend["loc"])

        self._axes_updated = True

    def _make_line(self, name, masks, hist):
        class Line:
            def __init__(self):
                self.data = None
                self.error = None
                self.masks = {}
                self.mpl_params = {}

        index = len(self._lines)
        line = Line()
        line.mpl_params = {
            key: get_line_param(key, index)
            for key in ["color", "marker", "linestyle", "linewidth"]
        }
        if self._mpl_line_params is not None:
            for key, item in self._mpl_line_params.items():
                if name in item:
                    line.mpl_params[key] = item[name]
        label = None
        if self._legend_labels and len(name) > 0:
            label = name

        if hist:
            line.data = self.ax.step(
                [1, 2], [1, 2],
                label=label,
                zorder=10,
                picker=self.picker,
                **{key: line.mpl_params[key]
                   for key in ["color", "linewidth"]})[0]
            for m in masks:
                line.masks[m] = self.ax.step([1, 2], [1, 2],
                                             linewidth=line.mpl_params["linewidth"] *
                                             3.0,
                                             color=self._mask_color,
                                             zorder=9)[0]
                # Abuse a mostly unused property `gid` of Line2D to
                # identify the line as a mask. We set gid to `onaxes`.
                # This is used by the profile viewer in the 2D plotter
                # to know whether to show the mask or not, depending on
                # whether the cursor is hovering over the 2D image or
                # not.
                line.masks[m].set_gid("onaxes")
        else:
            line.data = self.ax.plot([1, 2], [1, 2],
                                     label=label,
                                     zorder=10,
                                     picker=self.picker,
                                     **line.mpl_params)[0]
            for m in masks:
                line.masks[m] = self.ax.plot([1, 2], [1, 2],
                                             zorder=11,
                                             mec=self._mask_color,
                                             mfc="None",
                                             mew=3.0,
                                             linestyle="none",
                                             marker=line.mpl_params["marker"])[0]
                line.masks[m].set_gid("onaxes")

        if self.picker:
            line.data.set_pickradius(5.0)
        line.data.set_url(name)

        # Add error bars
        if self.errorbars[name]:
            line.error = self.ax.errorbar([1, 2], [1, 2],
                                          yerr=[1, 1],
                                          color=line.mpl_params["color"],
                                          zorder=10,
                                          fmt="none")
        if self.show_legend():
            self.ax.legend(loc=self.legend["loc"])
        return line

    def _preprocess_hist(self, name, vals):
        """
        Convert 1d data to be plotted to internal format, e.g., padding
        histograms and duplicating info for variances.
        """
        x = vals["values"]["x"]
        y = vals["values"]["y"]
        hist = len(x) != len(y)
        if hist:
            vals["values"]["y"] = np.concatenate((y[0:1], y))
            for key, mask in vals["masks"].items():
                vals["masks"][key] = np.concatenate((mask[0:1], mask))
            vals["variances"]["x"] = 0.5 * (x[1:] + x[:-1])
        else:
            vals["variances"]["x"] = x
        vals["variances"]["y"] = y
        return vals, hist

    def update_data(self, new_values):
        """
        Update the x and y positions of the data points when a new data slice
        is received for display.
        """
        xmin = np.Inf
        xmax = np.NINF
        for name in new_values:
            vals, hist = self._preprocess_hist(name, new_values[name])
            if name not in self._lines:
                self._lines[name] = self._make_line(name,
                                                    masks=vals['masks'].keys(),
                                                    hist=hist)
            line = self._lines[name]
            line.data.set_data(vals["values"]["x"], vals["values"]["y"])
            lab = vals["label"] if len(vals["label"]) > 0 else name
            line.label = f'{name}[{lab}]'  # used later if line is kept

            for m in vals["masks"]:
                line.masks[m].set_data(
                    vals["values"]["x"],
                    np.where(vals["masks"][m], vals["values"]["y"],
                             None).astype(np.float32))

            if self.errorbars[name]:
                coll = line.error.get_children()[0]
                coll.set_segments(
                    self._change_segments_y(vals["variances"]["x"],
                                            vals["variances"]["y"],
                                            vals["variances"]["e"]))
            coord = vals["values"]["x"]
            low = min(coord[0], coord[-1])
            high = max(coord[0], coord[-1])
            xmin = min(xmin, low)
            xmax = max(xmax, high)

        deltax = 0.05 * (xmax - xmin)
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self.ax.set_xlim([xmin - deltax, xmax + deltax])
        if self._axes_updated:
            self._axes_updated = False
            self.fig.tight_layout(rect=self.padding)

        self.draw()

    def keep_line(self, color, line_id, names=None):
        """
        Duplicate the current main line and give it an arbitrary color.
        Triggered by a `PlotPanel1d` keep button or a `keep_profile` event.
        """
        if names is None:
            names = self._lines
        for name in names:
            # The main line
            line = self._lines[name]
            self.ax.lines.append(cp.copy(line.data))
            self.ax.lines[-1].set_label(line.label)
            self.ax.lines[-1].set_url(line_id)
            self.ax.lines[-1].set_zorder(2)
            if self.ax.lines[-1].get_marker() == "None":
                self.ax.lines[-1].set_color(color)
            else:
                self.ax.lines[-1].set_markerfacecolor(color)
                self.ax.lines[-1].set_markeredgecolor("None")

            # The masks
            for m in self._lines[name].masks:
                self.ax.lines.append(cp.copy(self._lines[name].masks[m]))
                self.ax.lines[-1].set_url(line_id)
                self.ax.lines[-1].set_gid(m)
                self.ax.lines[-1].set_zorder(3)
                if self.ax.lines[-1].get_marker() != "None":
                    self.ax.lines[-1].set_zorder(3)
                else:
                    self.ax.lines[-1].set_zorder(1)

            if self.errorbars[name]:
                err = self._lines[name].error.get_children()
                self.ax.collections.append(cp.copy(err[0]))
                self.ax.collections[-1].set_color(color)
                self.ax.collections[-1].set_url(line_id)
                self.ax.collections[-1].set_zorder(2)

            if self.show_legend():
                self.ax.legend(loc=self.legend["loc"])
            self.draw()

    def remove_line(self, line_id, names=None):
        """
        Remove a previously saved line.
        Triggered by a `PlotPanel1d` remove button or a `remove_profile` event.
        """
        if names is None:
            names = self._lines
        for name in names:
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
        if mask_group in self._lines:
            msk = self._lines[mask_group].masks[mask_name]
            if msk.get_gid() == "onaxes":
                msk.set_visible(value)
        # Also toggle masks on additional lines created by keep button
        for line in self.ax.lines:
            if line.get_gid() == mask_name:
                line.set_visible(value)
        self.draw()

    def rescale_to_data(self, vmin=None, vmax=None):
        """
        Rescale y axis to the contents of the plot.
        """
        if (vmin is None) and (vmax is None):
            self.ax.autoscale(True)
            self.ax.relim()
            self.ax.autoscale_view()
        else:
            self.ax.set_ylim(vmin, vmax)
        self.draw()

    def show_legend(self):
        """
        Only display legend if there is least 1 line in the plot.
        """
        return self.legend["show"] and len(self.ax.get_legend_handles_labels()[1]) > 0

    def toggle_norm(self, norm=None, vmin=None, vmax=None):
        """
        Set yscale to either "log" or "linear", depending on norm.
        """
        self.norm = norm
        self.ax.set_yscale("log" if self.norm == "log" else "linear")
        self.draw()
