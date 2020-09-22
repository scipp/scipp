# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .controller import PlotController
from .model1d import PlotModel1d
from .panel1d import PlotPanel1d
from .profile import ProfileView
from .sciplot import SciPlot
from .tools import to_bin_edges, parse_params
from .view1d import PlotView1d
from .._utils import name_with_unit
from .._scipp import core as sc

# Other imports
import numpy as np
import copy as cp
import matplotlib.pyplot as plt
import ipywidgets as ipw
import warnings


def plot1d(scipp_obj_dict=None,
           axes=None,
           errorbars=None,
           masks={"color": "k"},
           filename=None,
           figsize=None,
           ax=None,
           mpl_line_params=None,
           logx=False,
           logy=False,
           logxy=False,
           grid=False,
           title=None):
    """
    Plot a 1D spectrum.

    Input is a Dataset containing one or more Variables.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    If the data contains more than one dimensions, sliders are added.
    """

    sp = SciPlot1d(scipp_obj_dict=scipp_obj_dict,
                   axes=axes,
                   errorbars=errorbars,
                   masks=masks,
                   ax=ax,
                   mpl_line_params=mpl_line_params,
                   logx=logx or logxy,
                   logy=logy or logxy,
                   grid=grid,
                   title=title)

    if filename is not None:
        sp.savefig(filename)

    return sp


class SciPlot1d(SciPlot):
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 errorbars=None,
                 masks=None,
                 ax=None,
                 pax=None,
                 mpl_line_params=None,
                 logx=False,
                 logy=False,
                 grid=False,
                 title=None):

        super().__init__()

        # The main controller module which contains the slider widgets
        self.controller = PlotController(scipp_obj_dict=scipp_obj_dict,
                                         axes=axes,
                                         masks=masks,
                                         logx=logx,
                                         logy=logy,
                                         errorbars=errorbars,
                                         button_options=['X'])

        # The model which takes care of all heavy calculations
        self.model = PlotModel1d(controller=self.controller,
                                 scipp_obj_dict=scipp_obj_dict)

        # The view which will display the 1d plot and send pick events back to
        # the controller
        self.view = PlotView1d(
            controller=self.controller,
            ax=ax,
            errorbars=self.controller.errorbars,
            title=title,
            unit=self.controller.params["values"][
                self.controller.name]["unit"],
            mask_params=self.controller.params["masks"][self.controller.name],
            mask_names=self.controller.mask_names,
            logx=logx,
            logy=logy,
            mpl_line_params=mpl_line_params,
            picker=5,
            grid=grid)

        # Profile view which displays an additional dimension as a 1d plot
        if self.controller.ndim > 1:
            self.profile = ProfileView(
                errorbars=self.controller.errorbars,
                ax=pax,
                unit=self.controller.params["values"][
                    self.controller.name]["unit"],
                mask_params=self.controller.params["masks"][
                    self.controller.name],
                mask_names=self.controller.mask_names,
                logx=logx,
                logy=logy,
                figsize=(config.plot.width / config.plot.dpi,
                         0.6 * config.plot.height / config.plot.dpi),
                is_profile=True)

        # An additional panel view with widgets to save/remove lines
        if self.controller.ndim > 1:
            self.panel = PlotPanel1d(controller=self.controller,
                                     data_names=list(scipp_obj_dict.keys()))

        # Connect controller to model, view, panel and profile
        self._connect_controller_members()

        # Call update_slice once to make the initial plot
        self.controller.update_axes()

        return
