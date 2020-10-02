# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .controller2d import PlotController2d
from .model2d import PlotModel2d
from .profile import ProfileView
from .sciplot import SciPlot
from .view2d import PlotView2d


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
                   figsize=figsize,
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

    return sp


class SciPlot2d(SciPlot):
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 ax=None,
                 cax=None,
                 figsize=None,
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

        super().__init__()

        # The main controller module which contains the slider widgets
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

        # The model which takes care of all heavy calculations
        self.model = PlotModel2d(controller=self.controller,
                                 scipp_obj_dict=scipp_obj_dict,
                                 resolution=resolution)

        # The view which will display the 2d image and send pick events back to
        # the controller
        self.view = PlotView2d(
            controller=self.controller,
            ax=ax,
            cax=cax,
            figsize=figsize,
            aspect=aspect,
            cmap=self.controller.params["values"][
                self.controller.name]["cmap"],
            norm=self.controller.params["values"][
                self.controller.name]["norm"],
            title=self.controller.name,
            cbar=self.controller.params["values"][
                self.controller.name]["cbar"],
            unit=self.controller.params["values"][
                self.controller.name]["unit"],
            mask_cmap=self.controller.params["masks"][
                self.controller.name]["cmap"],
            masks=self.controller.masks[self.controller.name],
            logx=logx,
            logy=logy)

        # Profile view which displays an additional dimension as a 1d plot
        if self.controller.ndim > 2:
            mask_params = self.controller.params["masks"][self.controller.name]
            mask_params["color"] = "k"
            pad = config.plot.padding
            pad[2] = 0.75
            self.profile = ProfileView(
                errorbars=self.controller.errorbars,
                ax=pax,
                unit=self.controller.params["values"][
                    self.controller.name]["unit"],
                mask_params=mask_params,
                masks=self.controller.masks,
                logx=logx,
                logy=logy,
                figsize=(1.3 * config.plot.width / config.plot.dpi,
                         0.6 * config.plot.height / config.plot.dpi),
                padding=pad,
                legend={"show": True, "loc": (1.02, 0.0)})

        # Connect controller to model, view, panel and profile
        self._connect_controller_members()

        # Call update_slice once to make the initial image
        self.controller.update_axes()

        return
