# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ... import DataArray, stddevs
from .tools import get_line_param
from .limits import find_limits, fix_empty_range, delta

from functools import reduce
import numpy as np
from numpy.typing import ArrayLike
from typing import Tuple


class Line:
    """
    """

    def __init__(self,
                 ax,
                 data,
                 number=0,
                 color=None,
                 linestyle=None,
                 marker=None,
                 linewidth=None,
                 norm: str = None,
                 mask_color: str = None,
                 errorbars: bool = True):

        self._ax = ax
        self._data = data
        self._line = None
        self._mask = None
        self._error = None
        self._errorbars = errorbars
        self._dim = None
        self._unit = None
        self.label = None

        self._mask_color = mask_color if mask_color is not None else 'k'

        self._dim = self._data.dim
        self._unit = self._data.unit
        self._coord = self._data.meta[self._dim]

        params = self._make_line_params(number=number,
                                        color=color,
                                        linestyle=linestyle,
                                        marker=marker,
                                        linewidth=linewidth)

        self._make_line(data=self._make_data(), errorbars=errorbars, params=params)

    def _make_line_params(self, number, **kwargs):
        return {
            key: get_line_param(key, number) if arg is None else arg
            for key, arg in kwargs.items()
        }

    def _make_line(self, data, errorbars, params):
        self.label = data["name"]
        has_mask = data["mask"] is not None
        mask_data_key = "mask" if has_mask else "values"

        if data["hist"]:
            self._line = self._ax.step(
                data["values"]["x"],
                data["values"]["y"],
                label=self.label,
                zorder=10,
                **{key: params[key]
                   for key in ["color", "linewidth"]})[0]

            self._mask = self._ax.step(data["values"]["x"],
                                       data[mask_data_key]["y"],
                                       linewidth=params["linewidth"] * 3.0,
                                       color=self._mask_color,
                                       zorder=9,
                                       visible=has_mask)[0]
        else:
            self._line = self._ax.plot(data["values"]["x"],
                                       data["values"]["y"],
                                       label=self.label,
                                       zorder=10,
                                       **params)[0]
            self._mask = self._ax.plot(data["values"]["x"],
                                       data[mask_data_key]["y"],
                                       zorder=11,
                                       mec=self._mask_color,
                                       mfc="None",
                                       mew=3.0,
                                       linestyle="none",
                                       marker=params["marker"],
                                       visible=has_mask)[0]

        # Add error bars
        if errorbars and ("e" in data["variances"]):
            self._error = self._ax.errorbar(data["variances"]["x"],
                                            data["variances"]["y"],
                                            yerr=data["variances"]["e"],
                                            color=params["color"],
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
        data['name'] = self._data.name
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
