# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from typing import TypeVar

import numpy as np

from ..core import DataArray, Dataset

_T = TypeVar('_T', bound=DataArray | Dataset)


def _to_slices(
    scipp_obj: _T,
    slice_sizes: dict[str, int],
    volume: int,
) -> dict[str, _T]:
    # Go through the dims that need to be collapsed, and create an array that
    # holds the range of indices for each dimension
    # Say we have [Y, 5], and [Z, 3], then dim_list will contain
    # [[0, 1, 2, 3, 4], [0, 1, 2]]
    dim_list = [np.arange(size, dtype=np.int32) for dim, size in slice_sizes.items()]
    # Next create a grid of indices
    # grid will contain
    # [ [[0, 1, 2, 3, 4], [0, 1, 2, 3, 4], [0, 1, 2, 3, 4]],
    #   [[0, 0, 0, 0, 0], [1, 1, 1, 1, 1], [2, 2, 2, 2, 2]] ]
    grid = np.meshgrid(*list(dim_list))
    # Reshape the grid to have a 2D array of length volume, i.e.
    # [ [0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4],
    #   [0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2] ]
    res = np.reshape(grid, (len(slice_sizes), volume))
    # Now make a master array which also includes the dimension labels, i.e.
    # [ [Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y, Y],
    #   [0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4],
    #   [Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z, Z],
    #   [0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2] ]
    slice_list = []
    for i, dim in enumerate(slice_sizes):
        slice_list.append([dim] * volume)
        slice_list.append(res[i])
    # Finally reshape the master array to look like
    # [ [[Y, 0], [Z, 0]], [[Y, 1], [Z, 0]],
    #   [[Y, 2], [Z, 0]], [[Y, 3], [Z, 0]],
    #   [[Y, 4], [Z, 0]], [[Y, 0], [Z, 1]],
    #   [[Y, 1], [Z, 1]], [[Y, 2], [Z, 1]],
    #   [[Y, 3], [Z, 1]],
    # ...
    # ]
    slice_lines = np.reshape(
        np.transpose(np.array(slice_list, dtype=np.dtype('O'))),
        (volume, len(slice_sizes), 2),
    )

    # Extract each entry from the slice_list
    all_slices = {}
    for line in slice_lines:
        vslice = scipp_obj
        key = ""
        for s in line:
            vslice = vslice[s[0], s[1]]  # type: ignore[assignment]
            key += f"{s[0]}:{s[1]}-"
        all_slices[key[:-1]] = vslice

    return all_slices


def collapse(scipp_obj: _T, keep: str) -> dict[str, _T]:
    """
    Slice down the input object until only the supplied `keep` dimension is
    left (effectively 'collapsing' all but one dimension), and return a
    `dict` of 1D slices. A common use for this is plotting spectra from
    detectors where most pixels contain noise, but one specific channel
    contains a strong signal. The `plot` function accepts a `dict` of data
    arrays.

    Parameters
    ----------
    scipp_obj:
        Dataset or DataArray to be split into slices.
    keep:
        Dimension to be preserved.

    Returns
    -------
    :
        A dictionary holding 1D slices of the input object.
    """

    dims = scipp_obj.dims
    shape = scipp_obj.shape

    # Gather list of dimensions that are to be collapsed
    volume = 1
    slice_sizes = {}
    for d, size in zip(dims, shape, strict=True):
        if d != keep:
            slice_sizes[d] = size
            volume *= size

    return _to_slices(scipp_obj, slice_sizes, volume)


def slices(scipp_obj: _T, dim: str) -> dict[str, _T]:
    """
    Slice input along given dim, and return all the slices in a `dict`.

    Parameters
    ----------
    scipp_obj:
        Dataset or DataArray to be split into slices.
    dim:
        Dimension along which to slice.

    Returns
    -------
    :
        A dictionary holding slices of the input object.
    """

    volume = scipp_obj.shape[scipp_obj.dims.index(dim)]
    slice_sizes = {dim: volume}

    return _to_slices(scipp_obj, slice_sizes, volume)
