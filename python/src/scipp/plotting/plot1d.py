# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .objects import Plot, make_params, make_profile
from .panel1d import PlotPanel1d
from .figure1d import PlotFigure1d


def plot1d(scipp_obj_dict,
           filename=None,
           labels=None,
           errorbars=None,
           masks=None,
           ax=None,
           pax=None,
           figsize=None,
           norm=None,
           scale=None,
           vmin=None,
           vmax=None,
           resolution=None,
           title=None,
           xlabel=None,
           ylabel=None,
           mpl_line_params=None,
           grid=False,
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

    dims = next(iter(scipp_obj_dict.values())).dims
    if len(dims) > 1:
        profile_figure = make_profile(ax=pax,
                                      mask_color=params['masks']['color'])
        # An additional panel view with widgets to save/remove lines
        panel = PlotPanel1d(data_names=list(scipp_obj_dict.keys()))
    else:
        profile_figure = None
        panel = None

    figure = PlotFigure1d(ax=ax,
                          figsize=figsize,
                          norm=norm,
                          title=title,
                          mask_color=params['masks']['color'],
                          mpl_line_params=mpl_line_params,
                          picker=True,
                          grid=grid,
                          xlabel=xlabel,
                          ylabel=ylabel,
                          legend=legend)

    sp = Plot(scipp_obj_dict=scipp_obj_dict,
              figure=figure,
              profile_figure=profile_figure,
              errorbars=errorbars,
              panel=panel,
              labels=labels,
              resolution=resolution,
              params=params,
              norm=norm,
              scale=scale,
              view_ndims=1)

    if filename is not None:
        sp.savefig(filename)
    else:
        return sp
