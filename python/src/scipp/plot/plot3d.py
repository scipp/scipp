# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .controller3d import PlotController3d
from .model3d import PlotModel3d
from .panel3d import PlotPanel3d
from .sciplot import SciPlot
from .view3d import PlotView3d


def plot3d(scipp_obj_dict=None,
           positions=None,
           axes=None,
           masks=None,
           filename=None,
           figsize=None,
           aspect=None,
           cmap=None,
           log=False,
           vmin=None,
           vmax=None,
           color=None,
           background="#f0f0f0",
           nan_color="#d3d3d3",
           pixel_size=1.0,
           tick_size=None,
           show_outline=True):
    """
    Plot a 3D point cloud through a N dimensional dataset.
    For every dimension above 3, a slider is created to adjust the position of
    the slice in that particular dimension.
    It is possible to add cut surfaces as cartesian, cylindrical or spherical
    planes.
    """

    sp = SciPlot3d(scipp_obj_dict=scipp_obj_dict,
                   positions=positions,
                   axes=axes,
                   masks=masks,
                   cmap=cmap,
                   log=log,
                   vmin=vmin,
                   vmax=vmax,
                   color=color,
                   aspect=aspect,
                   background=background,
                   nan_color=nan_color,
                   pixel_size=pixel_size,
                   tick_size=tick_size,
                   show_outline=show_outline)

    if filename is not None:
        sp.savefig(filename)

    return sp


class SciPlot3d(SciPlot):
    def __init__(self,
                 scipp_obj_dict=None,
                 positions=None,
                 axes=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 aspect=None,
                 background=None,
                 nan_color=None,
                 pixel_size=None,
                 tick_size=None,
                 show_outline=True):

        super().__init__()

        # The main controller module which contains the slider widgets
        self.controller = PlotController3d(scipp_obj_dict=scipp_obj_dict,
                                           axes=axes,
                                           masks=masks,
                                           cmap=cmap,
                                           log=log,
                                           vmin=vmin,
                                           vmax=vmax,
                                           color=color,
                                           positions=positions,
                                           pixel_size=pixel_size,
                                           button_options=['X', 'Y', 'Z'])

        # An additional panel view with widgets to control the cut surface
        # Note that the panel needs to be created before the model.
        self.panel = PlotPanel3d(controller=self.controller,
                                 pixel_size=pixel_size)

        # The model which takes care of all heavy calculations
        self.model = PlotModel3d(controller=self.controller,
                                 scipp_obj_dict=scipp_obj_dict,
                                 positions=positions,
                                 cut_options=self.panel.cut_options)

        # The view which will display the 3d scene and send pick events back to
        # the controller
        self.view = PlotView3d(
            controller=self.controller,
            cmap=self.controller.params["values"][
                self.controller.name]["cmap"],
            norm=self.controller.params["values"][
                self.controller.name]["norm"],
            unit=self.controller.params["values"][
                self.controller.name]["unit"],
            mask_cmap=self.controller.params["masks"][
                self.controller.name]["cmap"],
            mask_names=self.controller.mask_names[self.controller.name],
            nan_color=nan_color,
            pixel_size=pixel_size,
            tick_size=tick_size,
            background=background,
            show_outline=show_outline)

        # Connect controller to model, view, panel and profile
        self._connect_controller_members()

        # Call update_slice once to make the initial image
        self.controller.update_axes()

        return
