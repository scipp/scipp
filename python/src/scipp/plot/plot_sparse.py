# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from . import config
from .render import render_plot
from .sparse import visit_sparse_data
from .tools import axis_label, parse_colorbar
from .._scipp import core as sc

# Other imports
import plotly.graph_objs as go


def plot_sparse(input_data, ndim=0, sparse_dim=None, backend=None, logx=False,
                logy=False, logxy=False, weights=True, size=50.0,
                filename=None, axes=None, cb=None, opacity=1.0, **kwargs):
    """
    Produce a scatter plot from sparse data.
    """

    xmin, xmax, sparse_data, var, name, dims, ndims = visit_sparse_data(
        input_data, sparse_dim=sparse_dim, return_sparse_data=True,
        weights=weights)

    coords = var.coords

    # Parse colorbar
    cbar = parse_colorbar(cb, plotly=True)

    xyz = "xyz"
    if ndims < 3:
        plot_type = "scattergl"
    elif ndims == 3:
        plot_type = "scatter3d"
    else:
        raise RuntimeError("Scatter plots for sparse data support at most "
                           "3 dimensions.")
    data = dict(type=plot_type, mode='markers', name=name,
                marker={"line": {"color": '#ffffff',
                                 "width": 1},
                        "opacity": opacity})
    for i in range(ndims):
        data[xyz[i]] = sparse_data[ndims - 1 - i]
    if len(sparse_data) > ndims:

        # TODO: Having both color and size code for the weights leads to
        # strange in plotly, where it would seem some objects are either
        # modified or run out of scope. We will keep just the color for now.

        # if weights.count("size") > 0:
        #     # Copies are apparently required here
        #     data["marker"]["size"] = sparse_data[-1].copy()
        #     data["marker"]["sizemode"] = "area"
        #     data["marker"]["sizeref"] = 2*np.amax(sparse_data[-1])/(size**2)
        # if weights.count("color") > 0:

        if weights is not None:
            data["marker"]["color"] = sparse_data[-1]
            data["marker"]["colorscale"] = cbar["name"]
            data["marker"]["showscale"] = True
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
