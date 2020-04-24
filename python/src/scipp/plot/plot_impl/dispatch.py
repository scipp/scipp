# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from scipp.plot.plot_impl.plot_1d import plot_1d
from scipp.plot.plot_impl.plot_2d import plot_2d
from scipp.plot.plot_impl.plot_3d import plot_3d
from ..events import histogram_events_data
from scipp.plot.plot_impl.plot_request import PlotRequest, OneDPlotKwargs, TwoDPlotKwargs


def dispatch(request: PlotRequest, **kwargs):
    """
    Function to automatically dispatch the dict of scipp objects to the
    appropriate plotting function depending on its dimensions.
    """

    if request.ndims < 1:
        raise RuntimeError("Invalid number of dimensions for "
                           "plotting: {}".format(request.ndims))

    if request.bins is not None:
        sparse_dict = {}
        for key, obj in request.scipp_objs.items():
            sparse_dict[key] = obj
            for dim, bn in request.bins.items():
                sparse_dict[key] = histogram_events_data(
                    sparse_dict[key], dim, bn)
        scipp_obj_dict = sparse_dict



    if request.projection is None:
        if request.ndims < 3:
            request.projection = "{}d".format(request.ndims)
        else:
            request.projection = "2d"
    request.projection = request.projection.lower()

    if request.projection == "1d":
        request.user_kwargs = OneDPlotKwargs(**kwargs)
        return plot_1d(request=request)
    elif request.projection == "2d":
        request.user_kwargs = TwoDPlotKwargs(**kwargs)
        return plot_2d(request=request)
    elif request.projection == "3d":
        return plot_3d(scipp_obj_dict, **kwargs)
    else:
        raise RuntimeError("Wrong projection type. Expected either '2d' "
                           f"or '3d', got {request.projection}.")
