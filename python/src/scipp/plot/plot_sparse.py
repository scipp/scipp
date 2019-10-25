# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from . import config
from .tools import axis_label, render_plot, parse_colorbar
from .._scipp import core as sc

# Other imports
import numpy as np
import plotly.graph_objs as go
from itertools import product


def visit_sparse_data(input_data, sparse_dim, return_sparse_data=False,
                      weights=None):

    xmin =  1.0e30
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
                    scatter_array[i].append(np.ones_like(vals) * input_data.coords[dims[i]].values[ind[i]])
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
        bins = sc.Variable([sparse_dim], values=np.linspace(xmin, xmax, bins + 1),
                           unit=var.coords[sparse_dim].unit)
    elif isinstance(bins, np.ndarray):
        bins = sc.Variable([sparse_dim], values=bins,
                           unit=var.coords[sparse_dim].unit)
    elif isinstance(bins, sc.Variable):
        pass

    return sc.histogram(input_data, bins)


def plot_sparse(input_data, ndim=0, sparse_dim=None, backend=None, logx=False, logy=False, logxy=False,
                weights=True, size=50.0, filename=None, axes=None, cb=None, opacity=1.0, **kwargs):
    """
    Produce a scatter plot from sparse data.
    """

    xmin, xmax, sparse_data, var, name, dims, ndims = visit_sparse_data(
        input_data, sparse_dim=sparse_dim, return_sparse_data=True,
        weights=weights)

    coords = var.coords

    # Parse colorbar
    cbar = parse_colorbar(config.cb, cb, plotly=True)

    xyz = "xyz"
    if ndims < 3:
        plot_type = "scattergl"
    elif ndims == 3:
        plot_type = "scatter3d"
    else:
        raise RuntimeError("Scatter plots for sparse data support at most 3 dimensions.")
    data = dict(type=plot_type, mode='markers', name=name,
                marker={"line": {"color": '#ffffff',
                                 "width": 1},
                        "opacity": opacity
                       }
                )
    for i in range(ndims):
        data[xyz[i]] = sparse_data[ndims - 1 - i]
    if len(sparse_data) > ndims:
        # data["marker"] = dict()

        # TODO: Having both color and size code for the weights leads to
        # strange in plotly, where it would seem some objects are either
        # modified or run out of scope. We will keep just the color for now.

        # if weights.count("size") > 0:
        #     # Copies are apparently required here
        #     data["marker"]["size"] = sparse_data[-1].copy()
        #     data["marker"]["sizemode"] = "area"
        #     data["marker"]["sizeref"] = 2.0 * np.amax(sparse_data[-1])/(size**2)
        # if weights.count("color") > 0:

        if weights is not None:
            data["marker"]["color"] = sparse_data[-1]
            data["marker"]["colorscale"] = cbar["name"]
            data["marker"]["showscale"] =True
            data["marker"]["colorbar"] = {"title": "Weights",
                                          "titleside": "right"}

    layout = dict(
        xaxis=dict(title=axis_label(coords[sparse_dim])),
        yaxis=dict(title=axis_label(coords[dims[0]])),
        showlegend=True,
        legend=dict(x=0.0, y=1.15, orientation="h"),
        height=config.height
    )
    if logx or logxy:
        layout["xaxis"]["type"] = "log"
    if logy or logxy:
        layout["yaxis"]["type"] = "log"

    fig = go.Figure(data=[data], layout=layout)
    if ndims == 3:
        fig.update_layout(scene={"xaxis_title": axis_label(coords[sparse_dim]),
                                 "yaxis_title": axis_label(coords[dims[1]]),
                                 "zaxis_title": axis_label(coords[dims[0]])})

    render_plot(static_fig=fig, interactive_fig=fig, backend=backend,
                filename=filename)
    return

