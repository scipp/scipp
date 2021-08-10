# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet


def dispatch(scipp_obj_dict, ndim=0, projection=None, mpl_line_params=None, **kwargs):
    """
    Function to automatically dispatch the dict of DataArrays to the
    appropriate plotting function, depending on the number of dimensions.
    """

    if ndim < 1:
        raise RuntimeError("Invalid number of dimensions for "
                           "plotting: {}".format(ndim))

    # Dispatch the input to the different plotting functions
    if projection is None:
        if ndim < 3:
            projection = "{}d".format(ndim)
        else:
            projection = "2d"
    projection = projection.lower()

    if projection == "1d":
        from .plot1d import plot1d
        return plot1d(scipp_obj_dict, mpl_line_params=mpl_line_params, **kwargs)
    elif projection == "2d":
        from .plot2d import plot2d
        return plot2d(scipp_obj_dict, **kwargs)
    elif projection == "3d":
        from .plot3d import plot3d
        return plot3d(scipp_obj_dict, **kwargs)
    else:
        raise RuntimeError("Wrong projection type. Expected either '1d', "
                           "'2d', or '3d', got {}.".format(projection))
