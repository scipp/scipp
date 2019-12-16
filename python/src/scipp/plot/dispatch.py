# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .plot_1d import plot_1d
from .plot_2d import plot_2d
from .plot_3d import plot_3d
from .plot_sparse import plot_sparse
from .sparse import histogram_sparse_data


def dispatch(scipp_obj_dict, ndim=0, name=None, collapse=None, sparse_dim=None,
             bins=None, projection=None, mpl_line_params=None,  **kwargs):
    """
    Function to automatically dispatch the dict of scipp objects to the
    appropriate plotting function depending on its dimensions.
    """

    if ndim < 1:
        raise RuntimeError("Invalid number of dimensions for "
                           "plotting: {}".format(ndim))

    if sparse_dim is not None and bins is not None:
        scipp_obj_dict = histogram_sparse_data(scipp_obj_dict, sparse_dim, bins)

    if projection is None:
        if ndim < 3:
            projection = "{}d".format(ndim)
        else:
            projection = "2d"
    projection = projection.lower()

    if sparse_dim is not None and bins is None:
        return plot_sparse(scipp_obj_dict, ndim=ndim, sparse_dim=sparse_dim,
                           mpl_scatter_params=mpl_line_params, **kwargs)
    elif projection == "1d":
        return plot_1d(scipp_obj_dict, mpl_line_params=mpl_line_params, **kwargs)
    elif projection == "2d":
        return plot_2d(scipp_obj_dict, name=name, **kwargs)
    elif projection == "3d":
        return plot_3d(scipp_obj_dict, name=name, **kwargs)
    else:
        raise RuntimeError("Wrong projection type. Expected either '2d' "
                           "or '3d', got {}.".format(projection))
