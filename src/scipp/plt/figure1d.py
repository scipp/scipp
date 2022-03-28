# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import Variable, scalar, DataArray
from .tools import get_line_param, find_limits, fix_empty_range, vars_to_err
from ..utils import name_with_unit
from .figure import Figure

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


class Line:
    def __init__(self):
        self.data = None
        self.error = None
        self.mask = None
        self.mpl_params = {}


class Figure1d(Figure):
    """
    Class for 1 dimensional plots. This is used by both the `PlotView1d` for
    normal 1d plots, and the `PlotProfile`.

    `PlotFigure1d` can "keep" the currently displayed line, or "remove" a
    previously saved line.
    """
    def __init__(self,
                 norm: str = None,
                 mask_color: str = None,
                 legend: dict = None,
                 errorbars: bool = True,
                 **kwargs):

        super().__init__(**kwargs)

        self._errorbars = errorbars

        self._lines = {}
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

    def _make_line(self, mask: Union[dict, None], hist: bool, errorbars: bool) -> Line:
        index = len(self._lines)
        line = Line()
        line.mpl_params = {
            key: get_line_param(key, index)
            for key in ["color", "marker", "linestyle", "linewidth"]
        }
        label = None

        if hist:
            line.data = self._ax.step(
                [1, 2], [1, 2],
                label=label,
                zorder=10,
                **{key: line.mpl_params[key]
                   for key in ["color", "linewidth"]})[0]
            line.mask = self._ax.step([1, 2], [1, 2],
                                      linewidth=line.mpl_params["linewidth"] * 3.0,
                                      color=self._mask_color,
                                      zorder=9,
                                      visible=mask is not None)[0]
        else:
            line.data = self._ax.plot([1, 2], [1, 2],
                                      label=label,
                                      zorder=10,
                                      **line.mpl_params)[0]
            line.mask = self._ax.plot([1, 2], [1, 2],
                                      zorder=11,
                                      mec=self._mask_color,
                                      mfc="None",
                                      mew=3.0,
                                      linestyle="none",
                                      marker=line.mpl_params["marker"],
                                      visible=mask is not None)[0]

        # Add error bars
        if errorbars:
            line.error = self._ax.errorbar([1, 2], [1, 2],
                                           yerr=[1, 1],
                                           color=line.mpl_params["color"],
                                           zorder=10,
                                           fmt="none")
        return line

    def _preprocess_hist(self, vals: dict) -> Tuple[dict, bool]:
        """
        Convert 1d data to be plotted to internal format, e.g., padding
        histograms and duplicating info for variances.
        """
        x = vals["values"]["x"]
        y = vals["values"]["y"]
        hist = len(x) != len(y)
        if hist:
            vals["values"]["y"] = np.concatenate((y[0:1], y))
            if vals["mask"] is not None:
                for key, mask in vals["mask"].items():
                    vals["mask"][key] = np.concatenate((mask[0:1], mask))
            vals["variances"]["x"] = 0.5 * (x[1:] + x[:-1])
        else:
            vals["variances"]["x"] = x
        vals["variances"]["y"] = y
        return vals, hist

    def _make_data(self, new_values: DataArray) -> dict:
        out = {"values": {}, "variances": {}, "mask": None}
        out['name'] = new_values.name
        out['label'] = _make_label(new_values)
        out["values"]["x"] = new_values.meta[self._dim].values
        out["values"]["y"] = new_values.values
        if new_values.variances is not None:
            out["variances"]["e"] = vars_to_err(new_values.variances)
        if len(new_values.masks):
            one_mask = reduce(lambda a, b: a | b, new_values.masks.values()).values
            out["mask"] = {
                "x": out["values"]["x"],
                "y": np.where(one_mask, out["values"]["y"], None).astype(np.float32)
            }
        return out

    def update(self, new_values: DataArray, key: str, draw: bool = True):
        """
        Update the x and y positions of the data points when a new data slice
        is received for display.
        """
        self._dim = new_values.dim
        self._unit = new_values.unit
        self._coord = new_values.meta[self._dim]

        new_values = self._make_data(new_values)

        errorbars = self._errorbars and len(new_values["variances"])

        vals, hist = self._preprocess_hist(new_values)
        if key not in self._lines:
            self._lines[key] = self._make_line(mask=new_values['mask'],
                                               hist=hist,
                                               errorbars=errorbars)
        line = self._lines[key]
        line.data.set_data(new_values["values"]["x"], new_values["values"]["y"])
        lab = new_values["label"] if len(new_values["label"]) > 0 else key
        line.label = f'{key}[{lab}]'  # used later if line is kept

        if new_values["mask"] is not None:
            line.mask.set_data(new_values["mask"]["x"], new_values["mask"]["y"])
            line.mask.set_visible(True)
        else:
            line.mask.set_visible(False)

        if errorbars:
            coll = line.error.get_children()[0]
            coll.set_segments(
                self._change_segments_y(new_values["variances"]["x"],
                                        new_values["variances"]["y"],
                                        new_values["variances"]["e"]))

        if draw:
            self._rescale_to_data()
            self.draw()

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
        self._rescale_to_data()
        self.draw()

    def render(self):
        self._ax.set_xlabel(self._xlabel if self.
                            _xlabel is not None else name_with_unit(var=self._coord))
        self._ax.set_ylabel(self._ylabel if self._ylabel is not None else
                            name_with_unit(var=scalar(1, unit=self._unit), name=""))
        self._rescale_to_data()
        self.draw()
