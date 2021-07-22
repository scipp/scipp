# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .objects import make_params, make_profile, make_plot
from .model1d import PlotModel1d
from .panel1d import PlotPanel1d
from .view1d import PlotView1d
from .figure1d import PlotFigure1d
from .controller1d import PlotController1d


def plot1d(scipp_obj_dict, **kwargs):
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
    def builder(*,
                dims,
                norm=None,
                masks=None,
                ax=None,
                pax=None,
                figsize=None,
                vmin=None,
                vmax=None,
                title=None,
                xlabel=None,
                ylabel=None,
                mpl_line_params=None,
                grid=False,
                legend=None):
        out = {
            'view_ndims': 1,
            'model': PlotModel1d,
            'view': PlotView1d,
            'controller': PlotController1d
        }
        if masks is None:
            masks = {"color": "k"}
        params = make_params(norm=norm, vmin=vmin, vmax=vmax, masks=masks)
        out['vmin'] = params["values"]["vmin"]
        out['vmax'] = params["values"]["vmax"]
        if len(dims) > 1:
            out['profile_figure'] = make_profile(ax=pax,
                                                 mask_color=params['masks']['color'])
            # An additional panel view with widgets to save/remove lines
            out['panel'] = PlotPanel1d(data_names=list(scipp_obj_dict.keys()))

        out['figure'] = PlotFigure1d(ax=ax,
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
        return out

    return make_plot(builder, scipp_obj_dict, **kwargs)
