# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Other imports
import numpy as np


def _to_slices(scipp_obj, slice_dims, slice_shape, volume):

    # Create container to collect all 1D slices as 1D variables
    all_slices = dict()

    # Go through the dims that need to be collapsed, and create an array that
    # holds the range of indices for each dimension
    # Say we have [Dim.Y, 5], and [Dim.Z, 3], then dim_list will contain
    # [[0, 1, 2, 3, 4], [0, 1, 2]]
    dim_list = []
    for dim in slice_dims:
        dim_list.append(np.arange(slice_shape[dim], dtype=np.int32))
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
    for i, dim in enumerate(slice_dims):
        slice_list.append([dim] * volume)
        slice_list.append(res[i])
    # Finally reshape the master array to look like
    # [ [[Dim.Y, 0], [Dim.Z, 0]], [[Dim.Y, 1], [Dim.Z, 0]],
    #   [[Dim.Y, 2], [Dim.Z, 0]], [[Dim.Y, 3], [Dim.Z, 0]],
    #   [[Dim.Y, 4], [Dim.Z, 0]], [[Dim.Y, 0], [Dim.Z, 1]],
    #   [[Dim.Y, 1], [Dim.Z, 1]], [[Dim.Y, 2], [Dim.Z, 1]],
    #   [[Dim.Y, 3], [Dim.Z, 1]],
    # ...
    # ]
    slice_list = np.reshape(np.transpose(np.array(slice_list, dtype=np.dtype('O'))),
                            (volume, len(slice_dims), 2))

    # Extract each entry from the slice_list
    for i, line in enumerate(slice_list):
        vslice = scipp_obj
        key = ""
        for s in line:
            vslice = vslice[s[0], s[1]]
            key += "{}:{}-".format(str(s[0]), s[1])
        all_slices[key[:-1]] = vslice

    return all_slices


def collapse(scipp_obj, keep):
    """
    Slice down the input object until only the supplied `keep` dimension is
    left (effectively 'collapsing' all but one dimension), and return a
    `dict` of 1D slices. A common use for this is plotting spectra from
    detectors where most pixels contain noise, but one specific channel
    contains a strong signal. The `plot` function accepts a `dict` of data
    arrays.

    :param [scipp_obj]: Dataset or DataArray to be split into slices.
    :type [scipp_obj]: Dataset or DataArray
    :param [keep]: Dimension to be preserved.
    :type [dim]: str
    :return: A dictionary holding 1D slices of the input object.
    :rtype: dict
    """

    dims = scipp_obj.dims
    shape = scipp_obj.shape

    # Gather list of dimensions that are to be collapsed
    slice_dims = []
    volume = 1
    slice_shape = dict()
    for d, size in zip(dims, shape):
        if d != keep:
            slice_dims.append(d)
            slice_shape[d] = size
            volume *= size

    return _to_slices(scipp_obj, slice_dims, slice_shape, volume)


def slices(scipp_obj, dim):
    """
    Slice input along given dim, and return all the slices in a `dict`.

    :param [scipp_obj]: Dataset or DataArray to be split into slices.
    :type [scipp_obj]: Dataset or DataArray
    :param [dim]: Dimension along which to slice.
    :type [dim]: str
    :return: A dictionary holding slices of the input object.
    :rtype: dict
    """

    slice_dims = [dim]
    volume = scipp_obj.shape[scipp_obj.dims.index(dim)]
    slice_shape = {dim: volume}

    return _to_slices(scipp_obj, slice_dims, slice_shape, volume)
