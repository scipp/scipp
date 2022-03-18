# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .controller1d import Controller1d
from ..core import DataArray, Variable
from .params import make_params
from .plot import Plot
from .view1d import View1d

from typing import Any, Dict, Tuple


def plot1d(data_array_dict: Dict[str, DataArray],
           norm: str = None,
           masks: dict = None,
           ax: Any = None,
           figsize: Tuple[float, ...] = None,
           vmin: Variable = None,
           vmax: Variable = None,
           title: str = None,
           xlabel: str = None,
           ylabel: str = None,
           grid: bool = False,
           legend: dict = None,
           errorbars: bool = True,
           filename: str = None) -> Plot:
    """
    Plot one or more Scipp data objects as a 1 dimensional line plot.

    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    If the data contains more than one dimensions, sliders are added to
    navigate to extra dimensions.

    If the input data has more than 1 dimensions, a `PlotPanel` with additional
    buttons is displayed. This allow the duplication of the currently
    displayed line, a functionality inspired by the Superplot in the Lamp
    software.
    """

    params = make_params(norm=norm, vmin=vmin, vmax=vmax, masks=masks)

    view = View1d(ax=ax,
                  figsize=figsize,
                  title=title,
                  xlabel=xlabel,
                  ylabel=ylabel,
                  grid=grid,
                  norm=params["values"]["norm"],
                  legend=legend,
                  errorbars=errorbars)

    sp = Plot(models=data_array_dict,
              view=view,
              controller=Controller1d,
              vmin=params["values"]["vmin"],
              vmax=params["values"]["vmax"],
              view_ndims=1)

    if filename is not None:
        sp.savefig(filename)
    else:
        return sp
