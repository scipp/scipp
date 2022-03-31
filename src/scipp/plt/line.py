# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import Variable, scalar, DataArray
from .tools import get_line_param, find_limits, fix_empty_range, vars_to_err
from ..utils import name_with_unit
# from .figure import Figure

from functools import reduce
import numpy as np
from numpy.typing import ArrayLike
from typing import Tuple, Union
import warnings


def _make_label(array: DataArray) -> str:
    # TODO use formatter
    labels = []
    for dim, coord in array.meta.items():
        unit = '' if coord.unit == 'dimensionless' else f' {coord.unit}'
        if dim not in array.dims:
            labels.append(f'{dim}={coord.values.round(decimals=2)}{unit}')
    return ', '.join(labels)


# class Line:
#     def __init__(self):
#         self.data = None
#         self.error = None
#         self.mask = None
#         self.mpl_params = {}


class Line:
    """
    Class for 1 dimensional plots. This is used by both the `PlotView1d` for
    normal 1d plots, and the `PlotProfile`.

    `PlotFigure1d` can "keep" the currently displayed line, or "remove" a
    previously saved line.
    """
    def __init__(self,
                 ax,
                 data,
                 norm: str = None,
                 mask_color: str = None,
                 legend: dict = None,
                 errorbars: bool = True,
                 **kwargs):

        # super().__init__(**kwargs)

        self._ax = ax
        self._data = data

        # self._values = {}

        self._line = None
        self._mask = None
        self._error = None

        # self._mask = None
        self._mpl_params = {}

        self._errorbars = errorbars

        # self._lines = {}
        self._dim = None
        self._unit = None

        if legend is None:
            legend = {"show": True}
        elif isinstance(legend, bool):
            legend = {"show": legend}
        elif "show" not in legend:
            legend["show"] = True

        self._mask_color = mask_color if mask_color is not None else 'k'
        self._norm = norm
        self._xmin = np.inf
        self._xmax = np.NINF
        # self.legend = legend
        # if "loc" not in self.legend:
        #     self.legend["loc"] = 0

        self._dim = self._data.dim
        self._unit = self._data.unit
        self._coord = self._data.meta[self._dim]

        self._make_line(data=self._make_data(), errorbars=errorbars)

        # self._ax.set_xlabel(self._xlabel if self.
        #                     _xlabel is not None else name_with_unit(var=self._coord))
        # self._ax.set_ylabel(self._ylabel if self._ylabel is not None else
        #                     name_with_unit(var=scalar(1, unit=self._unit), name=""))

    # def _make_line(self, mask: Union[dict, None], hist: bool, errorbars: bool):
    def _make_line(self, data, errorbars):
        # index = len(self._lines)
        index = 0
        # line = Line()
        mpl_params = {
            key: get_line_param(key, index)
            for key in ["color", "marker", "linestyle", "linewidth"]
        }
        label = None

        has_mask = data["mask"] is not None
        mask_data_key = "mask" if has_mask else "values"

        if data["hist"]:
            self._line = self._ax.step(
                data["values"]["x"],
                data["values"]["y"],
                label=label,
                zorder=10,
                **{key: mpl_params[key]
                   for key in ["color", "linewidth"]})[0]

            self._mask = self._ax.step(data[mask_data_key]["x"],
                                       data[mask_data_key]["y"],
                                       linewidth=mpl_params["linewidth"] * 3.0,
                                       color=self._mask_color,
                                       zorder=9,
                                       visible=has_mask)[0]
        else:
            self._line = self._ax.plot(data["values"]["x"],
                                       data["values"]["y"],
                                       label=label,
                                       zorder=10,
                                       **mpl_params)[0]
            self._mask = self._ax.plot(data[mask_data_key]["x"],
                                       data[mask_data_key]["y"],
                                       zorder=11,
                                       mec=self._mask_color,
                                       mfc="None",
                                       mew=3.0,
                                       linestyle="none",
                                       marker=mpl_params["marker"],
                                       visible=has_mask)[0]

        # Add error bars
        if errorbars and ("e" in data["variances"]):
            self._error = self._ax.errorbar(data["variances"]["x"],
                                            data["variances"]["y"],
                                            yerr=data["variances"]["e"],
                                            color=mpl_params["color"],
                                            zorder=10,
                                            fmt="none")
        # return line

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
                for key, mask in data["mask"].items():
                    data["mask"][key] = np.concatenate((mask[0:1], mask))
            data["variances"]["x"] = 0.5 * (x[1:] + x[:-1])
        else:
            data["variances"]["x"] = x
        data["variances"]["y"] = y
        data["hist"] = hist
        return data

    def _make_data(self) -> dict:
        data = {"values": {}, "variances": {}, "mask": None}
        data['name'] = self._data.name
        data['label'] = _make_label(self._data)
        data["values"]["x"] = self._data.meta[self._dim].values
        data["values"]["y"] = self._data.values
        if self._data.variances is not None:
            data["variances"]["e"] = vars_to_err(self._data.variances)
        if len(self._data.masks):
            one_mask = reduce(lambda a, b: a | b, self._data.masks.values()).values
            data["mask"] = {
                "x": data["values"]["x"],
                "y": np.where(one_mask, data["values"]["y"], None).astype(np.float32)
            }
        return self._preprocess_hist(data)

    def update(self, new_values: DataArray, key: str, draw: bool = True):
        """
        Update the x and y positions of the data points when a new data slice
        is received for display.
        """
        # self._dim = new_values.dim
        # self._unit = new_values.unit
        # self._coord = new_values.meta[self._dim]
        self._data = new_values

        new_values = self._make_data(new_values)

        errorbars = self._errorbars and len(new_values["variances"])

        # vals, hist = self._preprocess_hist(new_values)
        # if key not in self._lines:
        #     self._lines[key] = self._make_line(mask=new_values['mask'],
        #                                        hist=hist,
        #                                        errorbars=errorbars)
        # line = self._lines[key]

        self._line.set_data(new_values["values"]["x"], new_values["values"]["y"])
        lab = new_values["label"] if len(new_values["label"]) > 0 else key
        self._label = f'{key}[{lab}]'  # used later if line is kept

        if new_values["mask"] is not None:
            self._mask.set_data(new_values["mask"]["x"], new_values["mask"]["y"])
            self._mask.set_visible(True)
        else:
            self._mask.set_visible(False)

        if errorbars:
            coll = self._error.get_children()[0]
            coll.set_segments(
                self._change_segments_y(new_values["variances"]["x"],
                                        new_values["variances"]["y"],
                                        new_values["variances"]["e"]))

        # if draw:
        #     self.draw()

    def _change_segments_y(self, x: ArrayLike, y: ArrayLike, e: ArrayLike) -> ArrayLike:
        """
        Update the positions of the errorbars when `update_data` is called.
        """
        arr1 = np.repeat(x, 2)
        arr2 = np.array([y - e, y + e]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)

    def _make_limits(self) -> Tuple[float, ...]:
        """
        """
        vmin = np.inf
        vmax = np.NINF
        xmin = np.inf
        xmax = np.NINF

        for line in self._lines.values():
            data = Variable(dims=[self._dim], values=line.data.get_ydata())
            ylims = fix_empty_range(find_limits(data, scale=self._norm)[self._norm])
            vmin = min(vmin, ylims[0].value)
            vmax = max(vmax, ylims[1].value)
            coord = Variable(dims=[self._dim], values=line.data.get_xdata())
            xlims = fix_empty_range(find_limits(coord)["linear"])
            xmin = min(xmin, xlims[0].value)
            xmax = max(xmax, xlims[1].value)

        # Add padding
        if self._norm == "log":
            delta = 10**(0.05 * np.log10(vmax / vmin))
            vmin /= delta
            vmax *= delta
        else:
            delta = 0.05 * (vmax - vmin)
            vmin -= delta
            vmax += delta
        return xmin, xmax, vmin, vmax

    def _rescale_to_data(self):
        """
        Rescale y axis to the contents of the plot.
        """
        xmin, xmax, vmin, vmax = self._make_limits()
        if self._user_vmin is not None:
            assert self._user_vmin.unit == self._unit
            self._vmin = self._user_vmin.value
        elif vmin < self._vmin:
            self._vmin = vmin
        if self._user_vmax is not None:
            assert self._user_vmax.unit == self._unit
            self._vmax = self._user_vmax.value
        elif vmax > self._vmax:
            self._vmax = vmax

        self._ax.set_ylim(self._vmin, self._vmax)

        if xmin < self._xmin:
            self._xmin = xmin
        if xmax > self._xmax:
            self._xmax = xmax

        deltax = 0.05 * (self._xmax - self._xmin)
        with warnings.catch_warnings():
            warnings.filterwarnings("ignore", category=UserWarning)
            self._ax.set_xlim([self._xmin - deltax, self._xmax + deltax])

    def toggle_norm(self, change: dict):
        """
        Set yscale to either "log" or "linear", depending on norm.
        """
        self._norm = "log" if change["new"] else "linear"
        self._ax.set_yscale(self._norm)
        self.draw()

    # def render(self):
    #     self._ax.set_xlabel(self._xlabel if self.
    #                         _xlabel is not None else name_with_unit(var=self._coord))
    #     self._ax.set_ylabel(self._ylabel if self._ylabel is not None else
    #                         name_with_unit(var=scalar(1, unit=self._unit), name=""))
    #     self.draw()

    # def draw(self):
    #     self._rescale_to_data()
    #     super().draw()
