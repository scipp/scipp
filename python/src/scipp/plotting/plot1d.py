# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .objects import Plot, make_params, make_errorbar_params, make_profile
from .panel1d import PlotPanel1d
from .view1d import PlotView1d
from .figure1d import PlotFigure1d


def plot1d(scipp_obj_dict,
           filename=None,
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
           resolution=None,
           scale=None,
           grid=False,
           title=None,
           xlabel=None,
           ylabel=None,
           legend=None):
    """
    Plot one or more Scipp data objects as a 1 dimensional line plot.

    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    If the data contains more than one dimensions, sliders are added to
    navigate to extra dimensions.

    If the input data has more than 1 dimensions, a `PlotPanel` with additional
    buttons is displayed. This allow the duplication of the currently
    displayed line, a functionality inspired by the Superplot in the Lamp
    software.
    """
    if masks is None:
        masks = {"color": "k"}

    params = make_params(norm=norm, vmin=vmin, vmax=vmax, masks=masks)
    errorbars = make_errorbar_params(scipp_obj_dict, errorbars)
    sp = Plot(scipp_obj_dict=scipp_obj_dict,
              labels=labels,
              norm=norm,
              scale=scale,
              view_ndims=1)
    # The view which will display the 1d plot and send pick events back to
    # the controller
    sp.view = PlotView1d(figure=PlotFigure1d(
        ax=ax,
        figsize=figsize,
        errorbars=errorbars,
        norm=norm,
        title=title,
        mask_color=params['masks']['color'],
        mpl_line_params=mpl_line_params,
        picker=True,
        grid=grid,
        xlabel=xlabel,
        ylabel=ylabel,
        legend=legend),
                         formatters=sp._formatters)

    # Profile view which displays an additional dimension as a 1d plot
    if len(sp.dims) > 1:
        sp.profile = make_profile(ax=pax,
                                  errorbars=errorbars,
                                  mask_color=params['masks']['color'])
        # An additional panel view with widgets to save/remove lines
        sp.panel = PlotPanel1d(data_names=list(scipp_obj_dict.keys()))

    sp.controller = sp._make_controller(norm=norm,
                                        scale=scale,
                                        resolution=resolution,
                                        params=params)

    # Render the figure once all components have been created.
    sp.render()
    if filename is not None:
        sp.savefig(filename)
    else:
        return sp
