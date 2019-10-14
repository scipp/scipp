# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import numpy as np
from .. import _scipp as sc
from . import config
from .plot_collapse import plot_collapse
# The need for importing matplotlib.pyplot here is a little strange, but the
# reason is the following. We want to delay the imports of plot_matplotlib and
# plot_plotly to inside dispatch_to_backend() in order to hide them from the
# user in the notebook. However, by doing this, the first time a plot is made
# it is not displayed. Re-runnnig the cell will display it. So when
# matplotlib is imported, some magic must happen which enables plots to appear
# in the notebook output, and apparently this needs to happen before the cell
# is executed. Importing matplotlib here is a workaround, for lack of a better
# fix. See here: https://github.com/jupyter/notebook/issues/3691
# One could also add %matplotlib inline to the notebooks, but that would also
# not be the prettiest solution.
# Finally, note that this workaround will not work if the import of scipp
# happens inside the same cell as the call to plot.
try:
    import matplotlib.pyplot as plt # noqa
except ImportError:  # Catch error in case matplotlib is not installed
    pass


def plot(input_data, collapse=None, backend=None, color=None, **kwargs):
    """
    Wrapper function to plot any kind of dataset
    """

    # Create a list of variables which will then be dispatched to the plot_auto
    # function.
    # Search through the variables and group the 1D datasets that have
    # the same coordinate axis.
    # tobeplotted is a dict that holds pairs of
    # [number_of_dimensions, DatasetSlice], or
    # [number_of_dimensions, [List of DatasetSlices]] in the case of
    # 1d sc.Data.
    # TODO: 0D data is currently ignored -> find a nice way of
    # displaying it?
    tp = type(input_data)
    if tp is sc.core.DataProxy or tp is sc.core.DataArray:
        ds = sc.core.Dataset()
        ds[input_data.name] = input_data
        input_data = ds
    if tp is not list:
        input_data = [input_data]

    tobeplotted = dict()
    for ds in input_data:
        for name, var in ds:
            coords = var.coords
            ndims = len(coords)
            if ndims == 1:
                lab = coords[var.dims[0]]
                # Make a unique key from the dataset id in case there are more
                # than one dataset with 1D variables with the same coordinates
                key = "{}_{}".format(str(id(ds)), lab)
                if key in tobeplotted.keys():
                    tobeplotted[key][1][name] = ds[name]
                else:
                    tobeplotted[key] = [ndims, {name: ds[name]}]
            elif ndims > 1:
                tobeplotted[name] = [ndims, ds[name]]

    # Plot all the subsets
    color_count = 0
    for key, val in tobeplotted.items():
        if val[0] == 1:
            if color is None:
                color = []
                for l in val[1].keys():
                    color.append(dispatch_to_backend(get_color=True,
                                                     backend=backend,
                                                     index=color_count))
                    color_count += 1
            elif not isinstance(color, list):
                color = [color]
            name = None
        else:
            color = None
            name = key
        if collapse is not None:
            plot_collapse(input_data=val[1], dim=collapse, name=name,
                          backend=backend, **kwargs)
        else:
            dispatch_to_backend(get_color=False, backend=backend,
                                input_data=val[1], ndim=val[0], name=name,
                                color=color, **kwargs)

    return


def dispatch_to_backend(get_color=False, backend=None, **kwargs):
    """
    Select the appropriate backend for plotting (plotly or matplotlib) and
    send the data to be plotted to the appropriate function.
    """

    if backend is None:
        backend = config.backend

    if backend == "matplotlib":
        from .matplotlib.plot_matplotlib import plot_matplotlib, get_mpl_color
        if get_color:
            return get_mpl_color(**kwargs)
        else:
            plot_matplotlib(**kwargs)
    elif backend == "plotly":
        from .plotly.plot_plotly import plot_plotly, get_plotly_color
        if get_color:
            return get_plotly_color(**kwargs)
        else:
            plot_plotly(**kwargs)
    else:
        raise RuntimeError("Unknown backend {}. Currently supported "
                           "backends are 'plotly' and "
                           "'matplotlib'".format(backend))
    return
