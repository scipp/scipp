# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import _scipp as sc
from .tools import get_color
from .dispatch import dispatch

# Other imports
import numpy as np


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
    ds = sc.core.Dataset()
    ds.coords[dim] = sc.core.Variable([dim], values=coords[dim].values)
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
        if ds_temp.variances is not None:
            variances = ds_temp.variances
        ds[key] = sc.core.Variable([dim], values=ds_temp.values,
                                   variances=variances)
        data[key] = ds[key]
        color.append(get_color(index=i))

    # Send the newly created dictionary of DataProxy to the plot_1d function
    return dispatch(input_data=data, ndim=1, backend=backend, color=color,
                    **kwargs)

    return
