# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .view import PlotView
from .toolbar import PlotToolbar1d
from .tools import get_line_param, find_limits, fix_empty_range
import numpy as np
import copy as cp
import warnings
from .. import config
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from .. import units
import ipywidgets as ipw


def _make_label(array):
    # TODO use formatter
    labels = []
    for dim, coord in array.meta.items():
        unit = '' if coord.unit == units.dimensionless else f' {coord.unit}'
        if dim not in array.dims:
            labels.append(f'{dim}={coord.values.round(decimals=2)}{unit}')
    return ', '.join(labels)


class PlotView1d(PlotView):
    """
    Class for 1 dimensional plots. This is used by both the `PlotView1d` for
    normal 1d plots, and the `PlotProfile`.

    `PlotFigure1d` can "keep" the currently displayed line, or "remove" a
    previously saved line.
    """
    def __init__(self,
                 ax=None,
                 title=None,
                 norm=None,
                 grid=False,
                 mask_color=None,
                 figsize=None,
                 legend=None,
                 bounding_box=None,
                 xlabel=None,
                 ylabel=None):

        super().__init__(ax=ax,
                         figsize=figsize,
                         title=title,
                         xlabel=xlabel,
                         ylabel=ylabel,
                         toolbar=PlotToolbar1d,
                         grid=grid)

        self.errorbars = {}

        self._lines = {}
        self._dim = None

        if legend is None:
            legend = {"show": True}
        elif isinstance(legend, bool):
            legend = {"show": legend}
        elif "show" not in legend:
            legend["show"] = True

        self._mask_color = mask_color if mask_color is not None else 'k'
        self.norm = norm
        self.legend = legend
        if "loc" not in self.legend:
            self.legend["loc"] = 0

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
        label = None

        if hist:
            line.data = self.ax.step(
                [1, 2], [1, 2],
                label=label,
                zorder=10,
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

        # Add error bars
        if self.errorbars[name]:
            line.error = self.ax.errorbar([1, 2], [1, 2],
                                          yerr=[1, 1],
                                          color=line.mpl_params["color"],
                                          zorder=10,
                                          fmt="none")
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

    def _make_masks(self, array, mask_info):
        if not mask_info:
            return {}
        masks = {}
        data = array.data
        base_mask = ones(sizes=data.sizes, dtype='int32', unit=None)
        for m in mask_info:
            if m in array.masks:
                msk = base_mask * Variable(
                    dims=array.masks[m].dims, unit=None, values=array.masks[m].values)
                masks[m] = msk.values
            else:
                masks[m] = None
        return masks

    def _make_data(self, new_values, mask_info=None):
        out = {}
        for name, array in new_values.items():
            self._dim = array.dims[0]  # should be same for all items
            values = {"values": {}, "variances": {}, "masks": {}}
            values['label'] = _make_label(array)
            values["values"]["x"] = array.meta[self._dim].values.ravel()
            values["values"]["y"] = array.values.ravel()
            if array.variances is not None:
                values["variances"]["e"] = vars_to_err(array.variances.ravel())
            # values["masks"] = self._make_masks(array, mask_info=mask_info[name])
            out[name] = values
        return out

    def update_data(self, new_values):
        """
        Update the x and y positions of the data points when a new data slice
        is received for display.
        """
        self._data = new_values

        raw_values = self._make_data(self._data)

        xmin = np.Inf
        xmax = np.NINF
        for name in raw_values:
            self.errorbars[name] = False
            vals, hist = self._preprocess_hist(name, raw_values[name])
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
        self.draw()

    def _change_segments_y(self, x, y, e):
        """
        Update the positions of the errorbars when `update_data` is called.
        """
        arr1 = np.repeat(x, 2)
        arr2 = np.array([y - e, y + e]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    def rescale_to_data(self, button=None):
        """
        Rescale y axis to the contents of the plot.
        """
        vmin = 1.0e30
        vmax = -1.0e30
        xmin = 1.0e30
        xmax = -1.0e30

        for name, array in self._data.items():
            ylims = fix_empty_range(find_limits(array.data, scale=self.norm)[self.norm])
            vmin = min(vmin, ylims[0].value)
            vmax = max(vmax, ylims[1].value)
            xlims = fix_empty_range(find_limits(array.meta[self._dim])["linear"])
            xmin = min(xmin, xlims[0].value)
            xmax = max(xmax, xlims[1].value)

        # Add padding
        if self.norm == "log":
            delta = 10**(0.05 * np.log10(vmax / vmin))
            vmin /= delta
            vmax *= delta
        else:
            delta = 0.05 * (vmax - vmin)
            vmin -= delta
            vmax += delta

        self.ax.set_ylim(vmin, vmax)

        deltax = 0.05 * (xmax - xmin)
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self.ax.set_xlim([xmin - deltax, xmax + deltax])

        self.draw()

    def toggle_norm(self, change):
        """
        Set yscale to either "log" or "linear", depending on norm.
        """
        self.norm = "log" if change["new"] else "linear"
        self.ax.set_yscale(self.norm)
        self.rescale_to_data()
