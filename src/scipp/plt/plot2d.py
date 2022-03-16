# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .params import make_params
from .plot import Plot
# from .model import Model
from .view2d import View2d
from .controller2d import Controller2d


def plot2d(data_array_dict,
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

    # model = Model(next(iter(scipp_obj_dict.values())))

    sp = Plot(
        models=data_array_dict,
        # model=model,
        view=view,
        controller=Controller2d,
        # norm=params["values"]["norm"],
        # masks=params["masks"],
        # ax=None,
        # cax=None,
        # figsize=None,
        # aspect=None,
        # cmap=params["values"]["cmap"],
        vmin=params["values"]["vmin"],
        vmax=params["values"]["vmax"],
        # title=None,
        # xlabel=None,
        # ylabel=None,
        # grid=False,
        view_ndims=2)
    if filename is not None:
        sp.savefig(filename)
    else:
        return sp
