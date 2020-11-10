# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .._scipp import core as sc
from .._utils import is_variable
import numpy as np


def dispatch(scipp_obj_dict,
             ndim=0,
             name=None,
             bins=None,
             projection=None,
             mpl_line_params=None,
             **kwargs):
    """
    Function to automatically dispatch the dict of DataArrays to the
    appropriate plotting function, depending on the number of dimensions.
    """

    if ndim < 1:
        raise RuntimeError("Invalid number of dimensions for "
                           "plotting: {}".format(ndim))

    for key, array in scipp_obj_dict.items():
        if sc.is_bins(array):
            if bins is not None:
                for dim, binning in bins.items():
                    coord = array.coords[dim]
                    if is_variable(binning):
                        edges = binning
                    elif isinstance(binning, int):
                        edges = sc.Variable([dim],
                                            values=np.linspace(
                                                coord[dim, 0].value,
                                                coord[dim, -1].value, binning),
                                            unit=coord.unit)
                    elif isinstance(binning, np.ndarray):
                        edges = sc.Variable([dim],
                                            values=binning,
                                            unit=coord.unit)
                    else:
                        raise RuntimeError(
                            "Unknown bins type: {}".format(binning))
                    array = sc.histogram(array, edges)
                scipp_obj_dict[key] = array
            else:
                scipp_obj_dict[key] = array.bins.sum()

    if projection is None:
        if ndim < 3:
            projection = "{}d".format(ndim)
        else:
            projection = "2d"
    projection = projection.lower()

    if projection == "1d":
        from .plot1d import plot1d
        return plot1d(scipp_obj_dict,
                      mpl_line_params=mpl_line_params,
                      **kwargs)
    elif projection == "2d":
        from .plot2d import plot2d
        return plot2d(scipp_obj_dict, **kwargs)
    elif projection == "3d":
        from .plot3d import plot3d
        return plot3d(scipp_obj_dict, **kwargs)
    else:
        raise RuntimeError("Wrong projection type. Expected either '1d', "
                           "'2d', or '3d', got {}.".format(projection))
