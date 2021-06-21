# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .controller1d import PlotController1d
from .model1d import PlotModel1d
from .objects import Plot
from .panel1d import PlotPanel1d
from .profile import PlotProfile
from .view1d import PlotView1d
from .figure1d import PlotFigure1d
from .widgets import PlotWidgets


def plot1d(*args, filename=None, **kwargs):
    """
    Plot one or more Scipp data objects as a 1 dimensional line plot.

    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    If the data contains more than one dimensions, sliders are added to
    navigate to extra dimensions.
    """
    sp = Plot1d(*args, **kwargs)
    if filename is not None:
        sp.savefig(filename)
    else:
        return sp


class Plot1d(Plot):
    """
    Class for 1 dimensional plots.

    If the input data has more than 1 dimensions, a `PlotPanel` with additional
    buttons is displayed. This allow the duplication of the currently
    displayed line, a functionality inspired by the Superplot in the Lamp
    software.
    """
    def __init__(self,
                 scipp_obj_dict=None,
                 labels=None,
                 errorbars=None,
                 masks=None,
                 ax=None,
                 pax=None,
                 figsize=None,
                 mpl_line_params=None,
                 norm=None,
                 vmin=None,
                 vmax=None,
                 scale=None,
                 grid=False,
                 title=None,
                 xlabel=None,
                 ylabel=None,
                 legend=None):

        if masks is None:
            masks = {"color": "k"}
        view_ndims = 1

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         labels=labels,
                         norm=norm,
                         vmin=vmin,
                         vmax=vmax,
                         errorbars=errorbars,
                         masks=masks,
                         view_ndims=view_ndims)

        # The model which takes care of all heavy calculations
        self.model = PlotModel1d(scipp_obj_dict=scipp_obj_dict, name=self.name)
        profile_model = PlotModel1d(scipp_obj_dict=scipp_obj_dict,
                                    name=self.name)

        # Create control widgets (sliders and buttons)
        self.widgets = PlotWidgets(dims=self.dims,
                                   formatters=self._formatters,
                                   ndim=view_ndims,
                                   dim_label_map=self.labels,
                                   masks=scipp_obj_dict)

        # The view which will display the 1d plot and send pick events back to
        # the controller
        self.view = PlotView1d(figure=PlotFigure1d(
            ax=ax,
            figsize=figsize,
            errorbars=self.errorbars,
            norm=norm,
            title=title,
            mask_color=self.params['masks']['color'],
            mpl_line_params=mpl_line_params,
            picker=True,
            grid=grid,
            xlabel=xlabel,
            ylabel=ylabel,
            legend=legend),
                               formatters=self._formatters)

        # Profile view which displays an additional dimension as a 1d plot
        if len(self.dims) > 1:
            pad = config.plot.padding.copy()
            pad[2] = 0.77
            self.profile = PlotProfile(
                errorbars=self.errorbars,
                ax=pax,
                mask_color=self.params['masks']['color'],
                figsize=(1.3 * config.plot.width / config.plot.dpi,
                         0.6 * config.plot.height / config.plot.dpi),
                padding=pad,
                legend={
                    "show": True,
                    "loc": (1.02, 0.0)
                })
            # An additional panel view with widgets to save/remove lines
            self.panel = PlotPanel1d(data_names=list(scipp_obj_dict.keys()))

        # The main controller module which contains the slider widgets
        self.controller = PlotController1d(dims=self.dims,
                                           name=self.name,
                                           vmin=self.params["values"]["vmin"],
                                           vmax=self.params["values"]["vmax"],
                                           norm=norm,
                                           scale=scale,
                                           widgets=self.widgets,
                                           model=self.model,
                                           profile_model=profile_model,
                                           view=self.view,
                                           panel=self.panel,
                                           profile=self.profile)

        # Render the figure once all components have been created.
        self.render(norm=norm)
