# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .engine_2d import PlotEngine2d
from .render import render_plot
# from .profiler import Profiler
from .tools import to_bin_edges, parse_params
from .widgets import PlotWidgets
from .._utils import name_with_unit
from .._scipp import core as sc
from .. import detail

# Other imports
import numpy as np
import ipywidgets as ipw
import matplotlib.pyplot as plt
from matplotlib.axes import Subplot
import warnings
import os


def plot2d(scipp_obj_dict=None,
            axes=None,
            masks=None,
            filename=None,
            figsize=None,
            ax=None,
            cax=None,
            aspect=None,
            cmap=None,
            log=False,
            vmin=None,
            vmax=None,
            color=None,
            logx=False,
            logy=False,
            logxy=False,
            resolution=None):
    """
    Plot a 2D slice through a N dimensional dataset. For every dimension above
    2, a slider is created to adjust the position of the slice in that
    particular dimension.
    """

    sp = SciPlot2d(scipp_obj_dict=scipp_obj_dict,
                  axes=axes,
                  masks=masks,
                  ax=ax,
                  cax=cax,
                  aspect=aspect,
                  cmap=cmap,
                  log=log,
                  vmin=vmin,
                  vmax=vmax,
                  color=color,
                  logx=logx or logxy,
                  logy=logy or logxy,
                  resolution=resolution)

    if filename is not None:
        sp.fig.savefig(filename, bbox_inches="tight")

    # if ax is None:
    #     render_plot(figure=sv.fig, widgets=sv.vbox, filename=filename)

    return sp


class SciPlot2d:
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 ax=None,
                 cax=None,
                 aspect=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 logx=False,
                 logy=False,
                 resolution=None):


        self.presenter = PlotPresenter()

        self.model = PlotModel2d(presenter=self.presenter,
                         scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color)
                         # aspect=aspect)
                         # button_options=['X', 'Y'])

        self.controller = PlotController(presenter=self.presenter, #engine=self.engine,
                         button_options=['X', 'Y'])

        self.view = PlotView2d(presenter=self.presenter,
            ax=ax, cax=cax, aspect=aspect,
            cmap=self.model.params["values"][self.model.name]["cmap"],
            norm=self.model.params["values"][self.model.name]["norm"],
            title=self.model.name,
            cbar=self.model.params["values"][self.model.name]["cbar"],
            unit=self.model.params["values"][self.model.name]["unit"],
            mask_cmap=self.model.params["masks"][self.model.name]["cmap"],
            mask_name=self.model.mask_names)

        # Profile view
        self.profile = None



        return

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        # widgets_ = [self.figure, self.widgets]
        # if self.overview["additional_widgets"] is not None:
        #     wdgts.append(self.overview["additional_widgets"])
        return ipw.VBox([self.figure, self.widgets.container])

