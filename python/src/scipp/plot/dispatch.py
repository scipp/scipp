# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .._utils import histogram_events_data


def dispatch(scipp_obj_dict,
             ndim=0,
             name=None,
             bins=None,
             projection=None,
             mpl_line_params=None,
             **kwargs):
    """
    Function to automatically dispatch the dict of scipp objects to the
    appropriate plotting function depending on its dimensions.
    """

    if ndim < 1:
        raise RuntimeError("Invalid number of dimensions for "
                           "plotting: {}".format(ndim))

    if bins is not None:
        events_dict = {}
        for key, obj in scipp_obj_dict.items():
            events_dict[key] = obj
            for dim, bn in bins.items():
                events_dict[key] = histogram_events_data(
                    events_dict[key], dim, bn)
        scipp_obj_dict = events_dict

    if projection is None:
        if ndim < 3:
            projection = "{}d".format(ndim)
        else:
            projection = "2d"
    projection = projection.lower()

    if projection == "1d":
        from .plot_1d import plot_1d
        return plot_1d(scipp_obj_dict,
                       mpl_line_params=mpl_line_params,
                       **kwargs)
    elif projection == "2d":
        from .plot2d import plot2d
        return plot2d(scipp_obj_dict, **kwargs)
    elif projection == "3d":
        from .plot_3d import plot_3d
        return plot_3d(scipp_obj_dict, **kwargs)
    elif projection == "profile":
        from .profiler import profiler
        return profiler(scipp_obj_dict, **kwargs)
    else:
        raise RuntimeError("Wrong projection type. Expected either '1d', "
                           "'2d', '3d', or 'profile', "
                           "got {}.".format(projection))
