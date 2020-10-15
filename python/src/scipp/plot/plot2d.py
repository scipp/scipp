# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .controller2d import PlotController2d
from .model2d import PlotModel2d
from .profile import ProfileView
from .sciplot import SciPlot
from .view2d import PlotView2d
from .widgets import PlotWidgets


def plot2d(*args, filename=None, **kwargs):
    """
    Plot a 2D slice through a N dimensional dataset.
    For every dimension above 2, a slider is created to adjust the position
    of the slice in that particular dimension.
    """

    sp = SciPlot2d(*args, **kwargs)

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
                 pax=None,
                 figsize=None,
                 aspect=None,
                 cmap=None,
                 norm=None,
                 scale=None,
                 vmin=None,
                 vmax=None,
                 resolution=None):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         cmap=cmap,
                         norm=norm,
                         vmin=vmin,
                         vmax=vmax,
                         masks=masks,
                         view_ndims=2)

        # Create control widgets (sliders and buttons)
        self.widgets = PlotWidgets(axes=self.axes,
                                   ndim=self.ndim,
                                   name=self.name,
                                   dim_to_shape=self.dim_to_shape,
                                   masks=self.masks,
                                   button_options=['x', 'y'])

        # The model which takes care of all heavy calculations
        self.model = PlotModel2d(scipp_obj_dict=scipp_obj_dict,
                                 axes=self.axes,
                                 name=self.name,
                                 dim_to_shape=self.dim_to_shape,
                                 dim_label_map=self.dim_label_map,
                                 resolution=resolution)

        # The view which will display the 2d image and send pick events back to
        # the controller
        self.view = PlotView2d(ax=ax,
                               cax=cax,
                               figsize=figsize,
                               aspect=aspect,
                               cmap=self.params["values"][self.name]["cmap"],
                               norm=self.params["values"][self.name]["norm"],
                               title=self.name,
                               cbar=self.params["values"][self.name]["cbar"],
                               unit=self.params["values"][self.name]["unit"],
                               masks=self.masks[self.name])

        # Profile view which displays an additional dimension as a 1d plot
        if self.ndim > 2:
            pad = config.plot.padding.copy()
            pad[2] = 0.77
            self.profile = ProfileView(
                errorbars=self.errorbars,
                ax=pax,
                unit=self.params["values"][self.name]["unit"],
                masks=self.masks,
                figsize=(1.3 * config.plot.width / config.plot.dpi,
                         0.6 * config.plot.height / config.plot.dpi),
                padding=pad,
                legend={
                    "show": True,
                    "loc": (1.02, 0.0)
                })

        # The main controller module which connects and controls all the parts
        self.controller = PlotController2d(
            axes=self.axes,
            name=self.name,
            dim_to_shape=self.dim_to_shape,
            coord_shapes=self.coord_shapes,
            vmin=self.params["values"][self.name]["vmin"],
            vmax=self.params["values"][self.name]["vmax"],
            norm=norm,
            scale=scale,
            widgets=self.widgets,
            model=self.model,
            view=self.view,
            profile=self.profile)

        # Call update_slice once to make the initial image
        self.controller.update_axes()
