# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .._scipp import core as sc

# Other imports
import numpy as np
from itertools import product


def visit_sparse_data(input_data, sparse_dim, return_sparse_data=False,
                      weights=None):

    xmin = 1.0e30
    xmax = -1.0e30
    dims = input_data.dims
    shapes = input_data.shape
    ndims = len(dims)
    # Remove the sparse dim from dims
    isparse = dims.index(sparse_dim)
    dims.pop(isparse)
    shapes.pop(isparse)

    # Construct tuple of ranges over dimension shapes
    if ndims > 1:
        indices = tuple()
        for i in range(ndims - 1):
            indices += range(shapes[i]),
    else:
        indices = [0],

    # Get default variable to gain access to spare coordinate
    name, var = next(iter(input_data))
    # Check if weights are present
    data_exists = False
    if weights is not None:
        data_exists = var.data is not None

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
        vslice = var
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
                        input_data.coords[dims[i]].values[ind[i]])
                scatter_array[ndims - 1].append(vals)
                if data_exists:
                    scatter_array[ndims].append(vslice.values)

    if return_sparse_data:
        for i in range(ndims + data_exists):
            scatter_array[i] = np.concatenate(scatter_array[i])
        return xmin, xmax, scatter_array, var, name, dims, ndims
    else:
        return xmin, xmax


def histogram_sparse_data(input_data, sparse_dim, bins):
    var = next(iter(input_data))[1]
    if isinstance(bins, bool):
        bins = 256
    if isinstance(bins, int):
        # Find min and max
        xmin, xmax = visit_sparse_data(input_data, sparse_dim)
        dx = (xmax - xmin) / float(bins)
        # Add padding
        xmin -= 0.5 * dx
        xmax += 0.5 * dx
        bins = sc.Variable([sparse_dim],
                           values=np.linspace(xmin, xmax, bins + 1),
                           unit=var.coords[sparse_dim].unit)
    elif isinstance(bins, np.ndarray):
        bins = sc.Variable([sparse_dim], values=bins,
                           unit=var.coords[sparse_dim].unit)
    elif isinstance(bins, sc.Variable):
        pass

    return sc.histogram(input_data, bins)
