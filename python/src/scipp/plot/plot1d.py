# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .controller1d import PlotController1d
from .model1d import PlotModel1d
from .panel1d import PlotPanel1d
from .profile import PlotProfile
from .sciplot import SciPlot
from .view1d import PlotView1d
from .widgets import PlotWidgets


def plot1d(*args, filename=None, **kwargs):
    """
    Plot one or more Scipp data objects as a 1 dimensional line plot.

    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    If the data contains more than one dimensions, sliders are added to
    navigate to extra dimensions.
    """
    sp = SciPlot1d(*args, **kwargs)
    if filename is not None:
        sp.savefig(filename)
    return sp


class SciPlot1d(SciPlot):
    """
    Class for 1 dimensional plots.

    If the input data has more than 1 dimensions, a `PlotPanel` with additional
    buttons is displayed. This allow the duplication of the currently
    displayed line, a functionality inspired by the Superplot in the Lamp
    software.
    """
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 errorbars=None,
                 masks={"color": "k"},
                 ax=None,
                 pax=None,
                 figsize=None,
                 mpl_line_params=None,
                 norm=None,
                 vmin=None,
                 vmax=None,
                 scale=None,
                 grid=False,
                 title=None):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         axes=axes,
                         norm=norm,
                         vmin=vmin,
                         vmax=vmax,
                         errorbars=errorbars,
                         masks=masks,
                         view_ndims=1)

        self.widgets = PlotWidgets(axes=self.axes,
                                   ndim=self.ndim,
                                   name=self.name,
                                   dim_to_shape=self.dim_to_shape,
                                   masks=self.masks,
                                   button_options=['x'])

        # The model which takes care of all heavy calculations
        self.model = PlotModel1d(scipp_obj_dict=scipp_obj_dict,
                                 axes=self.axes,
                                 name=self.name,
                                 dim_to_shape=self.dim_to_shape,
                                 dim_label_map=self.dim_label_map)

        # The view which will display the 1d plot and send pick events back to
        # the controller
        self.view = PlotView1d(ax=ax,
                               figsize=figsize,
                               errorbars=self.errorbars,
                               norm=norm,
                               title=title,
                               unit=self.params["values"][self.name]["unit"],
                               masks=self.masks,
                               mpl_line_params=mpl_line_params,
                               picker=True,
                               grid=grid)

        # Profile view which displays an additional dimension as a 1d plot
        if self.ndim > 1:
            pad = config.plot.padding.copy()
            pad[2] = 0.77
            self.profile = PlotProfile(
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

        # An additional panel view with widgets to save/remove lines
        if self.ndim > 1:
            self.panel = PlotPanel1d(data_names=list(scipp_obj_dict.keys()))

        # The main controller module which contains the slider widgets
        self.controller = PlotController1d(
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
            panel=self.panel,
            profile=self.profile)

        # Call update_slice once to make the initial plot
        self.controller.update_axes()
