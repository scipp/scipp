# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet


def dispatch(input_data, ndim=0, name=None, backend=None, collapse=None,
             sparse_dim=None, bins=None, projection="2d", **kwargs):
    """
    Function to automatically dispatch the input dataset to the appropriate
    plotting function depending on its dimensions
    """

    if ndim < 1:
        raise RuntimeError("Invalid number of dimensions for "
                           "plotting: {}".format(ndim))

    if backend == "matplotlib" or backend == "matplotlib:quiet":

        from .plot_matplotlib import plot_1d, plot_2d
        if ndim == 1:
            return plot_1d(input_data, **kwargs)
        elif ndim == 2:
            return plot_2d(input_data, name=name, **kwargs)
        elif ndim > 2:
            raise RuntimeError("Plotting for 3 and more dimensions in "
                               "matplotlib is not available. Please supply a "
                               "1D or 2D dataset by slicing your object. "
                               "Alternatively, try using the plotly "
                               "interactive backend instead by setting "
                               "scipp.plot.config.backend = 'interactive'.")

    else:

        # Delayed imports
        from .plot_1d import plot_1d
        from .plot_2d import plot_2d
        from .plot_3d import plot_3d
        from .plot_sparse import histogram_sparse_data, plot_sparse

        if sparse_dim is not None:
            if bins is not None:
                input_data = histogram_sparse_data(input_data, sparse_dim, bins)
            else:
                plot_sparse(input_data, ndim=ndim, sparse_dim=sparse_dim, backend=backend, **kwargs)
                return

        if ndim == 1:
            plot_1d(input_data, backend=backend, **kwargs)
        elif projection.lower() == "2d":
            plot_2d(input_data, name=name, ndim=ndim, backend=backend,
                    **kwargs)
        elif projection.lower() == "3d":
            plot_3d(input_data, name=name, ndim=ndim, backend=backend,
                    **kwargs)
        else:
            raise RuntimeError("Wrong projection type. Expected either '2d' "
                               "or '3d', got {}.".format(projection))

    return
