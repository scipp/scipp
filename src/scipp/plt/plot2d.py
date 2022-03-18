# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ..core import DataArray, Variable
from .controller import Controller
from .params import make_params
from .plot import Plot
from .view2d import View2d

from typing import Any, Dict, Tuple


def plot2d(data_array_dict: Dict[str, DataArray],
           norm: str = None,
           masks: dict = None,
           ax: Any = None,
           cax: Any = None,
           figsize: Tuple[float, ...] = None,
           aspect: str = None,
           cmap: str = None,
           vmin: Variable = None,
           vmax: Variable = None,
           title: str = None,
           xlabel: str = None,
           ylabel: str = None,
           grid: bool = False,
           filename: str = None) -> Plot:
    """
    Plot a 2d slice through a N dimensional dataset.
    For every dimension above 2, a slider is created to adjust the position
    of the slice in that particular dimension.
    """

    params = make_params(cmap=cmap, norm=norm, vmin=vmin, vmax=vmax, masks=masks)

    view = View2d(ax=ax,
                  cax=cax,
                  figsize=figsize,
                  title=title,
                  xlabel=xlabel,
                  ylabel=ylabel,
                  grid=grid,
                  aspect=aspect,
                  cmap=params["values"]["cmap"],
                  norm=params["values"]["norm"],
                  masks=params["masks"])

    sp = Plot(models=data_array_dict,
              view=view,
              controller=Controller,
              vmin=params["values"]["vmin"],
              vmax=params["values"]["vmax"],
              view_ndims=2)
    if filename is not None:
        sp.savefig(filename)
    else:
        return sp
