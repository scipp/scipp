# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .controller2d import PlotController2d
from .model2d import PlotModel2d
# from .render import render_plot
from .lineplot import LinePlot
from .tools import to_bin_edges, parse_params
from .view2d import PlotView2d
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
        sp.savefig(filename)

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
                 pax=None,
                 aspect=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 logx=False,
                 logy=False,
                 resolution=None):


        self.controller = PlotController2d(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         masks=masks,
                         cmap=cmap,
                         log=log,
                         vmin=vmin,
                         vmax=vmax,
                         color=color,
                         logx=logx,
                         logy=logy,
            button_options=['X', 'Y'])

        self.model = PlotModel2d(controller=self.controller,
            scipp_obj_dict=scipp_obj_dict,
            resolution=resolution)

        # Connect controller to model
        self.controller.model = self.model
                         # aspect=aspect)
                         # button_options=['X', 'Y'])

        # self.controller.widgets = PlotWidgets(controller=self.controller, #engine=self.engine,
        #                  button_options=['X', 'Y'])

        self.view = PlotView2d(controller=self.controller,
            ax=ax, cax=cax, aspect=aspect,
            cmap=self.controller.params["values"][self.controller.name]["cmap"],
            norm=self.controller.params["values"][self.controller.name]["norm"],
            title=self.controller.name,
            cbar=self.controller.params["values"][self.controller.name]["cbar"],
            unit=self.controller.params["values"][self.controller.name]["unit"],
            mask_cmap=self.controller.params["masks"][self.controller.name]["cmap"],
            mask_names=self.controller.mask_names[self.controller.name],
            logx=logx,
            logy=logy)

        self.controller.view = self.view

        # Profile view
        self.profile = None
        if self.controller.ndim > 2:
            mask_params = self.controller.params["masks"][self.controller.name]
            mask_params["color"] = "k"
            self.profile = LinePlot(errorbars=self.controller.errorbars,
                 ax=pax,
                 unit=self.controller.params["values"][self.controller.name]["unit"],
                 mask_params=mask_params,
                 mask_names=self.controller.mask_names,
                 logx=logx,
                 logy=logy,
                 figsize=(config.plot.width / config.plot.dpi,
                         0.6 * config.plot.height / config.plot.dpi),
                 is_profile=True)



            # self.profile = ProfileView(
            #     errorbars=self.controller.errorbars,
            #     unit=self.controller.params["values"][self.controller.name]["unit"],
            #     mask_params=self.controller.params["masks"][self.controller.name],
            #     mask_names=self.controller.mask_names)
            # # controller=self.controller
            #      # ax=None,
            #      # errorbars=None,
            #      # title=None,
            #      # unit=None,
            #      # logx=False,
            #      # logy=False,
            #      # mask_params=None,
            #      # mask_names=None,
            #      # mpl_line_params=None,
            #      # grid=False)

        self.controller.profile = self.profile


        # Call update_slice once to make the initial image
        self.controller.update_axes()


        return

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        # widgets_ = [self.figure, self.widgets]
        # if self.overview["additional_widgets"] is not None:
        #     wdgts.append(self.overview["additional_widgets"])
        return ipw.VBox([self.view._to_widget(), self.profile._to_widget(), self.controller._to_widget()])

    def savefig(self, filename=None):
        self.view.savefig(filename=filename)
