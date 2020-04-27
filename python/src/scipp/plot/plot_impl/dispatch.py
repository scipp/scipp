# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from typing import List, Union

from scipp.plot.plot_impl.plot_1d import plot_1d
from scipp.plot.plot_impl.plot_2d import plot_2d
from scipp.plot.plot_impl.plot_3d import plot_3d
from ..events import histogram_events_data
from scipp.plot.plot_impl.plot_request import PlotRequest, OneDPlotKwargs, TwoDPlotKwargs, ThreeDPlotKwargs


def dispatch(request: Union[PlotRequest, List[PlotRequest]],
             **kwargs):
    """
    Function to automatically dispatch the dict of scipp objects to the
    appropriate plotting function depending on its dimensions.
    """
    if isinstance(request, list):
        if not all(r.projection == request[0].projection for r in request):
            # In the future we could look at this, but get subplots working for simple cases first
            raise ValueError("All projection types (i.e. 1D/2D/3D) in a subplot must match")

        for r in request:
            _prepare_dispatch(r, **kwargs)
        projection = request[0].projection
    else:
        _prepare_dispatch(request, **kwargs)
        projection = request.projection

    if projection == "1d":
        return plot_1d(to_plot=request)
    elif projection == "2d":
        return plot_2d(to_plot=request)
    elif projection == "3d":
        return plot_3d(to_plot=request)
    else:
        raise RuntimeError("Wrong projection type. Expected either '2d' "
                           f"or '3d', got {request.projection}.")


def _prepare_dispatch(request, **kwargs):
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
        request.scipp_objs = sparse_dict

    if request.projection is None:
        if request.ndims < 3:
            request.projection = "{}d".format(request.ndims)
        else:
            request.projection = "2d"
    request.projection = request.projection.lower()

    if request.projection == "1d":
        request.user_kwargs = OneDPlotKwargs(**kwargs)
    elif request.projection == "2d":
        request.user_kwargs = TwoDPlotKwargs(**kwargs)
    elif request.projection == "3d":
        request.user_kwargs = ThreeDPlotKwargs(**kwargs)
    else:
        raise RuntimeError("Unknown projection internally used, please contact the development team.")
