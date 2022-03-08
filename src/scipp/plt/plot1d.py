# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .objects import make_params, make_profile, Plot
from .model1d import PlotModel1d
# from .panel1d import PlotPanel1d
from .view1d import PlotView1d
from .controller1d import PlotController1d


def plot1d(scipp_obj_dict,
           norm=None,
           masks=None,
           ax=None,
           figsize=None,
           vmin=None,
           vmax=None,
           title=None,
           xlabel=None,
           ylabel=None,
           grid=False,
           legend=None,
           filename=None):
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

    params = make_params(norm=norm, vmin=vmin, vmax=vmax, masks=masks)

    sp = Plot(
        scipp_obj_dict=scipp_obj_dict,
        model=PlotModel1d,
        view=PlotView1d,
        controller=PlotController1d,
        norm=params["values"]["norm"],
        # masks=None,
        ax=None,
        figsize=None,
        vmin=params["values"]["vmin"],
        vmax=params["values"]["vmax"],
        title=None,
        xlabel=None,
        ylabel=None,
        grid=False,
        view_ndims=1)

    if filename is not None:
        sp.savefig(filename)
    else:
        return sp
