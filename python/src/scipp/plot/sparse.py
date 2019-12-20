# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .._scipp import core as sc

# Other imports
import numpy as np
from itertools import product


def visit_sparse_data(data_array, sparse_dim, return_sparse_data=False,
                      weights=None):

    xmin = 1.0e30
    xmax = -1.0e30
    dims = data_array.dims
    shapes = data_array.shape
    ndims = len(dims)
    # Remove the sparse dim from dims
    isparse = dims.index(sparse_dim)
    dims.pop(isparse)

    # Construct tuple of ranges over dimension shapes
    if ndims > 1:
        indices = tuple()
        for i in range(ndims - 1):
            indices += range(shapes[i]),
    else:
        indices = [0],

    # Check if weights are present
    data_exists = False
    if weights is not None:
        data_exists = data_array.data is not None

    # Prepare scatter data container
    if return_sparse_data:
        scatter_array = []
        for i in range(ndims):
            scatter_array.append([])
        # Append the weights associated to the sparse coordinate
        if data_exists:
            scatter_array.append([])

    # Now construct all indices combinations using itertools
    for ind in product(*indices):
        # And for each indices combination, slice the original
        # data down to the sparse dimension
        vslice = data_array
        if ndims > 1:
            for i in range(ndims - 1):
                vslice = vslice[dims[i], ind[i]]

        vals = vslice.coords[sparse_dim].values
        if len(vals) > 0:
            xmin = min(xmin, np.nanmin(vals))
            xmax = max(xmax, np.nanmax(vals))
            if return_sparse_data:
                for i in range(ndims - 1):
                    scatter_array[i].append(
                        np.ones_like(vals) *
                        data_array.coords[dims[i]].values[ind[i]])
                scatter_array[ndims - 1].append(vals)
                if data_exists:
                    scatter_array[ndims].append(vslice.values)

    if return_sparse_data:
        for i in range(ndims + data_exists):
            scatter_array[i] = np.concatenate(scatter_array[i])
        return xmin, xmax, scatter_array, dims, ndims
    else:
        return xmin, xmax


def make_bins(data_array=None, sparse_dim=None, bins=None, dim=None):
    """
    Input bins can be different things:
    - a bool (True): then a default number of 256 bins is made
    - an integer: this denotes the number of bins. The input data is scanned
      and the entire range of sparse data is covered, using the specified
      number of bins.
    - a numpy array: denotes the bin edges to be used
    - a Variable: denotes bin edges, can be used directly in the histogramming
      function.
    """
    if isinstance(bins, sc.Variable):
        pass
    else:
        if sparse_dim is not None:
            bin_dim = sparse_dim
        elif dim is not None:
            bin_dim = dim
        else:
            raise RuntimeError("Must specify either sparse_dim or dim for "
                               "making bins.")

        if isinstance(bins, bool):
            if bins:
                bins = 256
            else:
                bins = None
        if isinstance(bins, int):
            # Find min and max
            if sparse_dim is not None:
                xmin, xmax = visit_sparse_data(data_array, sparse_dim)
            else:
                xmin = np.amin(data_array.coords[dim].values)
                xmax = np.amax(data_array.coords[dim].values)

            dx = (xmax - xmin) / float(bins)
            # Add padding
            xmin -= 0.5 * dx
            xmax += 0.5 * dx
            bins = sc.Variable([bin_dim],
                               values=np.linspace(xmin, xmax, bins + 1),
                               unit=data_array.coords[bin_dim].unit)
        elif isinstance(bins, np.ndarray):
            bins = sc.Variable([bin_dim], values=bins,
                               unit=data_array.coords[bin_dim].unit)
        else:
            raise RuntimeError("Unknown bins type: {}".format(bins))
    return bins


def histogram_sparse_data(data_array, sparse_dim, bins):
    """
    Return a DataArray containing histogrammed sparse data, from specified
    sparse dimensions and bins. See make_bins for more details.
    """
    return sc.histogram(data_array, make_bins(data_array=data_array,
                                              sparse_dim=sparse_dim,
                                              bins=bins))
