# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import numpy as np
import scipp as sp
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


class Config:
    """
    A small class to hold various configuration parameters
    """

    def __init__(self):

        # Select plotting backend
        self.backend = "plotly"
        # The colorbar properties
        self.cb = {"name": "viridis", "log": False, "min": None, "max": None,
                   "min_var": None, "max_var": None}
        # The default image height (in pixels)
        self.height = 600
        # The default image width (in pixels)
        self.width = 950


config = Config()


def plot(input_data, collapse=None, backend=None, **kwargs):
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
    # 1d sp.Data.
    # TODO: 0D data is currently ignored -> find a nice way of
    # displaying it?
    tp = type(input_data)
    if tp is sp.DataProxy:
        ds = sp.Dataset()
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
            color = []
            for l in val[1].keys():
                color.append(dispatch_to_backend(get_color=True,
                                                 backend=backend,
                                                 index=color_count))
                color_count += 1
            name = None
        else:
            color = None
            name = key
        if collapse is not None:
            plot_collapse(input_data=val[1], dim=collapse, name=name,
                          backend=backend, config=config, **kwargs)
        else:
            dispatch_to_backend(get_color=False, backend=backend,
                                input_data=val[1], ndim=val[0], name=name,
                                color=color, config=config, **kwargs)

    return


def dispatch_to_backend(get_color=False, backend=None, **kwargs):
    """
    Select the appropriate backend for plotting (plotly or matplotlib) and
    send the data to be plotted to the appropriate function.
    """

    if backend is None:
        backend = config.backend

    if backend == "matplotlib":
        from .plot_matplotlib import plot_matplotlib, get_mpl_color
        if get_color:
            return get_mpl_color(**kwargs)
        else:
            plot_matplotlib(**kwargs)
    elif backend == "plotly":
        from .plot_plotly import plot_plotly, get_plotly_color
        if get_color:
            return get_plotly_color(**kwargs)
        else:
            plot_plotly(**kwargs)
    else:
        raise RuntimeError("Unknown backend {}. Currently supported "
                           "backends are 'plotly' and "
                           "'matplotlib'".format(backend))
    return


def plot_collapse(input_data, dim=None, name=None, filename=None, backend=None,
                  **kwargs):
    """
    Collapse higher dimensions into a 1D plot.
    """

    dims = input_data.dims
    shape = input_data.shape
    coords = input_data.coords

    # Gather list of dimensions that are to be collapsed
    slice_dims = []
    volume = 1
    slice_shape = dict()
    for d, size in zip(dims, shape):
        if d != dim:
            slice_dims.append(d)
            slice_shape[d] = size
            volume *= size

    # Create temporary Dataset
    ds = sp.Dataset()
    ds.coords[dim] = sp.Variable([dim], values=coords[dim].values)
    # A dictionary to hold the DataProxy objects
    data = dict()

    # Go through the dims that need to be collapsed, and create an array that
    # holds the range of indices for each dimension
    # Say we have [Dim.Y, 5], and [Dim.Z, 3], then dim_list will contain
    # [[0, 1, 2, 3, 4], [0, 1, 2]]
    dim_list = []
    for l in slice_dims:
        dim_list.append(np.arange(slice_shape[l], dtype=np.int32))
    # Next create a grid of indices
    # grid will contain
    # [ [[0, 1, 2, 3, 4], [0, 1, 2, 3, 4], [0, 1, 2, 3, 4]],
    #   [[0, 0, 0, 0, 0], [1, 1, 1, 1, 1], [2, 2, 2, 2, 2]] ]
    grid = np.meshgrid(*[x for x in dim_list])
    # Reshape the grid to have a 2D array of length volume, i.e.
    # [ [0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4],
    #   [0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2] ]
    res = np.reshape(grid, (len(slice_dims), volume))
    # Now make a master array which also includes the dimension labels, i.e.
    # [ [Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y,
    #    Dim.Y, Dim.Y, Dim.Y, Dim.Y, Dim.Y],
    #   [    0,     1,     2,     3,     4,     0,     1,     2,     3,     4,
    #        0,     1,     2,     3,     4],
    #   [Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z,
    #    Dim.Z, Dim.Z, Dim.Z, Dim.Z, Dim.Z],
    #   [    0,     0,     0,     0,     0,     1,     1,     1,     1,     1,
    #        2,     2,     2,     2,     2] ]
    slice_list = []
    for i, l in enumerate(slice_dims):
        slice_list.append([l] * volume)
        slice_list.append(res[i])
    # Finally reshape the master array to look like
    # [ [[Dim.Y, 0], [Dim.Z, 0]], [[Dim.Y, 1], [Dim.Z, 0]],
    #   [[Dim.Y, 2], [Dim.Z, 0]], [[Dim.Y, 3], [Dim.Z, 0]],
    #   [[Dim.Y, 4], [Dim.Z, 0]], [[Dim.Y, 0], [Dim.Z, 1]],
    #   [[Dim.Y, 1], [Dim.Z, 1]], [[Dim.Y, 2], [Dim.Z, 1]],
    #   [[Dim.Y, 3], [Dim.Z, 1]],
    # ...
    # ]
    slice_list = np.reshape(
        np.transpose(slice_list), (volume, len(slice_dims), 2))

    # Extract each entry from the slice_list, make temporary dataset and add to
    # input dictionary for plot_1d
    color = []
    for i, line in enumerate(slice_list):
        ds_temp = input_data
        key = ""
        for s in line:
            ds_temp = ds_temp[s[0], s[1]]
            key += "{}-{}-".format(str(s[0]), s[1])
        # Add variances
        variances = None
        if ds_temp.has_variances:
            variances = ds_temp.variances
        ds[key] = sp.Variable([dim], values=ds_temp.values,
                              variances=variances)
        data[key] = ds[key]
        color.append(dispatch_to_backend(get_color=True, backend=backend,
                                         index=i))

    # Send the newly created dictionary of DataProxy to the plot_1d function
    dispatch_to_backend(get_color=False, backend=backend, input_data=data,
                        ndim=1, color=color, **kwargs)

    return
