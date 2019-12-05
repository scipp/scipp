# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .plot_1d import plot_1d
from .plot_2d import plot_2d
from .plot_3d import plot_3d
from .plot_sparse import plot_sparse
from .sparse import histogram_sparse_data


def dispatch(input_data, ndim=0, name=None, backend=None, collapse=None,
             sparse_dim=None, bins=None, projection=None, color=None,
             **kwargs):
    """
    Function to automatically dispatch the input dataset to the appropriate
    plotting function depending on its dimensions
    """

    if ndim < 1:
        raise RuntimeError("Invalid number of dimensions for "
                           "plotting: {}".format(ndim))

    if sparse_dim is not None and bins is not None:
        input_data = histogram_sparse_data(input_data, sparse_dim, bins)

    # if backend == "matplotlib" or backend == "matplotlib:quiet":

    #     from .plot_matplotlib import plot_1d, plot_2d, plot_sparse
    #     if sparse_dim is not None and bins is None:
    #         return plot_sparse(input_data, sparse_dim=sparse_dim,
    #                            color=color, **kwargs)
    #     elif ndim == 1:
    #         return plot_1d(input_data, color=color, **kwargs)
    #     elif ndim == 2:
    #         return plot_2d(input_data, name=name, **kwargs)
    #     elif ndim > 2:
    #         raise RuntimeError("Plotting for 3 and more dimensions in "
    #                            "matplotlib is not available. Please supply a "
    #                            "1D or 2D dataset by slicing your object. "
    #                            "Alternatively, try using the plotly "
    #                            "interactive backend instead by setting "
    #                            "scipp.config.plot.backend = 'interactive'.")

    # elif backend == "interactive" or backend == "static":

        # # Delayed imports
        # from .plot_1d import plot_1d
        # from .plot_2d import plot_2d
        # from .plot_3d import plot_3d
        # from .plot_sparse import plot_sparse

    if projection is None:
        if ndim < 3:
            projection = "{}d".format(ndim)
        else:
            projection = "2d"
    projection = projection.lower()

    if sparse_dim is not None and bins is None:
        return plot_sparse(input_data, ndim=ndim, sparse_dim=sparse_dim,
                    backend=backend, color=color, **kwargs)
    elif projection == "1d":
        return plot_1d(input_data, backend=backend, color=color, **kwargs)
    elif projection == "2d":
        return plot_2d(input_data, name=name, **kwargs)
    elif projection == "3d" or projection == "volume":
        return plot_3d(input_data, name=name,
                       volume=(projection == "volume"), **kwargs)
    else:
        raise RuntimeError("Wrong projection type. Expected either '2d' "
                           "or '3d', got {}.".format(projection))

    # else:
    #     raise RuntimeError("Unknown backend {}. Currently supported "
    #                        "backends are 'interactive', 'static', "
    #                        "'matplotlib' and "
    #                        "'matplotlib:quiet'".format(backend))

