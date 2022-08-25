# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ... import DataArray, stddevs
from .limits import find_limits, fix_empty_range, delta

from functools import reduce
import numpy as np
from numpy.typing import ArrayLike
from typing import Tuple
from matplotlib.lines import Line2D


class Line:
    """
    """

    def __init__(self,
                 ax,
                 data,
                 number=0,
                 mask_color: str = None,
                 errorbars: bool = True,
                 **kwargs):

        self._ax = ax
        self._data = data
        self._line = None
        self._mask = None
        self._error = None
        self._errorbars = errorbars
        self._dim = None
        self._unit = None
        self.label = data.name

        self._mask_color = mask_color if mask_color is not None else 'k'

        self._dim = self._data.dim
        self._unit = self._data.unit
        self._coord = self._data.meta[self._dim]

        aliases = {'ls': 'linestyle', 'lw': 'linewidth', 'c': 'color'}
        for key, alias in aliases.items():
            if key in kwargs:
                kwargs[alias] = kwargs.pop(key)

        self._make_line(data=self._make_data(),
                        number=number,
                        errorbars=errorbars,
                        **kwargs)

    def _make_line(self, data, errorbars, number, **kwargs):
        has_mask = data["mask"] is not None
        mask_data_key = "mask" if has_mask else "values"

        default_step_style = {
            'linestyle': 'solid',
            'linewidth': 1.5,
            'color': f'C{number}'
        }
        markers = list(Line2D.markers.keys())
        default_plot_style = {
            'linestyle': 'none',
            'linewidth': 1.5,
            'marker': markers[(number + 2) % len(markers)],
            'color': f'C{number}'
        }

        if data["hist"]:
            self._line = self._ax.step(data["values"]["x"],
                                       data["values"]["y"],
                                       label=self.label,
                                       zorder=10,
                                       **{
                                           **default_step_style,
                                           **kwargs
                                       })[0]

            self._mask = self._ax.step(data["values"]["x"], data[mask_data_key]["y"])[0]
            self._mask.update_from(self._line)
            self._mask.set_color(self._mask_color)
            self._mask.set_linewidth(self._mask.get_linewidth() * 3)
            self._mask.set_zorder(self._mask.get_zorder() - 1)
            self._mask.set_visible(has_mask)
        else:
            self._line = self._ax.plot(data["values"]["x"],
                                       data["values"]["y"],
                                       label=self.label,
                                       zorder=10,
                                       **{
                                           **default_plot_style,
                                           **kwargs
                                       })[0]
            self._mask = self._ax.plot(data["values"]["x"],
                                       data[mask_data_key]["y"],
                                       zorder=11,
                                       mec=self._mask_color,
                                       mfc="None",
                                       mew=3.0,
                                       linestyle="none",
                                       marker=self._line.get_marker(),
                                       visible=has_mask)[0]

        # Add error bars
        if errorbars and ("e" in data["variances"]):
            self._error = self._ax.errorbar(data["variances"]["x"],
                                            data["variances"]["y"],
                                            yerr=data["variances"]["e"],
                                            color=self._line.get_color(),
                                            zorder=10,
                                            fmt="none")

    def _preprocess_hist(self, data: dict) -> Tuple[dict, bool]:
        """
        Convert 1d data to be plotted to internal format, e.g., padding
        histograms and duplicating info for variances.
        """
        x = data["values"]["x"]
        y = data["values"]["y"]
        hist = len(x) != len(y)
        if hist:
            data["values"]["y"] = np.concatenate((y[0:1], y))
            if data["mask"] is not None:
                data["mask"]["y"] = np.concatenate(
                    (data["mask"]["y"][0:1], data["mask"]["y"]))
            data["variances"]["x"] = 0.5 * (x[1:] + x[:-1])
        else:
            data["variances"]["x"] = x
        data["variances"]["y"] = y
        data["hist"] = hist
        return data

    def _make_data(self) -> dict:
        data = {"values": {}, "variances": {}, "mask": None}
        data["values"]["x"] = self._data.meta[self._dim].values
        data["values"]["y"] = self._data.values
        if self._data.variances is not None:
            data["variances"]["e"] = stddevs(self._data.data).values
        if len(self._data.masks):
            one_mask = reduce(lambda a, b: a | b, self._data.masks.values()).values
            data["mask"] = {
                "y": np.where(one_mask, data["values"]["y"], None).astype(np.float32)
            }
        return self._preprocess_hist(data)

    def update(self, new_values: DataArray):
        """
        Update the x and y positions of the data points when a new data slice
        is received for display.
        """
        self._data = new_values
        new_values = self._make_data()

        self._line.set_data(new_values["values"]["x"], new_values["values"]["y"])
        if new_values["mask"] is not None:
            self._mask.set_data(new_values["values"]["x"], new_values["mask"]["y"])
            self._mask.set_visible(True)
        else:
            self._mask.set_visible(False)

        if self._errorbars and ("e" in new_values["variances"]):
            coll = self._error.get_children()[0]
            coll.set_segments(
                self._change_segments_y(new_values["variances"]["x"],
                                        new_values["variances"]["y"],
                                        new_values["variances"]["e"]))

    def _change_segments_y(self, x: ArrayLike, y: ArrayLike, e: ArrayLike) -> ArrayLike:
        """
        Update the positions of the errorbars when `update_data` is called.
        """
        arr1 = np.repeat(x, 2)
        arr2 = np.array([y - e, y + e]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    def get_limits(self, xscale, yscale) -> Tuple[float, ...]:
        """
        """
        xmin, xmax = fix_empty_range(
            find_limits(self._data.meta[self._dim], scale=xscale))
        ymin, ymax = fix_empty_range(find_limits(self._data.data, scale=yscale))

        # Add padding
        deltax = delta(xmin, xmax, 0.03, xscale)
        if xscale == "log":
            xmin = xmin / deltax
            xmax = xmax * deltax
        else:
            xmin = xmin - deltax
            xmax = xmax + deltax
        deltay = delta(ymin, ymax, 0.03, yscale)
        if yscale == "log":
            ymin = ymin / deltay
            ymax = ymax * deltay
        else:
            ymin = ymin - deltay
            ymax = ymax + deltay

        return xmin, xmax, ymin, ymax
