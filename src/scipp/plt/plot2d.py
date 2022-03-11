# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .objects import make_params, Plot
from .model import Model
from .view2d import PlotView2d
from .controller2d import PlotController2d


def plot2d(scipp_obj_dict,
           norm=None,
           masks=None,
           ax=None,
           cax=None,
           pax=None,
           figsize=None,
           aspect=None,
           cmap=None,
           vmin=None,
           vmax=None,
           title=None,
           xlabel=None,
           ylabel=None,
           grid=False,
           filename=None):
    """
    Plot a 2d slice through a N dimensional dataset.
    For every dimension above 2, a slider is created to adjust the position
    of the slice in that particular dimension.
    """

    params = make_params(cmap=cmap, norm=norm, vmin=vmin, vmax=vmax, masks=masks)

    sp = Plot(scipp_obj_dict=scipp_obj_dict,
              model=Model,
              view=PlotView2d,
              controller=PlotController2d,
              norm=params["values"]["norm"],
              masks=params["masks"],
              ax=None,
              cax=None,
              figsize=None,
              aspect=None,
              cmap=params["values"]["cmap"],
              vmin=params["values"]["vmin"],
              vmax=params["values"]["vmax"],
              title=None,
              xlabel=None,
              ylabel=None,
              grid=False,
              view_ndims=2)
    if filename is not None:
        sp.savefig(filename)
    else:
        return sp
