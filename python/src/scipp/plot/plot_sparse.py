# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .render import render_plot
from .sparse import visit_sparse_data
from .tools import axis_label, parse_colorbar

# Other imports
# import plotly.graph_objs as go
import matplotlib.pyplot as plt
try:
    import ipyvolume as ipv
except:
    pass



def plot_sparse(input_data, ndim=0, sparse_dim=None, backend=None, logx=False,
                logy=False, logxy=False, weights=True, size=50.0,
                filename=None, axes=None, cb=None, opacity=1.0, color=None):
    """
    Produce a scatter plot from sparse data.
    """

    xmin, xmax, sparse_data, var, name, dims, ndims = visit_sparse_data(
        input_data, sparse_dim=sparse_dim, return_sparse_data=True,
        weights=weights)

    coords = var.coords

    # Parse colorbar
    cbar = parse_colorbar(cb)

    xyz = "xyz"
    if ndims < 3:
        fig, ax = plt.subplots(1, 1, figsize=(config.width/config.dpi,
                                              config.height/config.dpi),
                               dpi=config.dpi)
        params = dict(label=name, edgecolors="#ffffff", c=color)
        xs = sparse_data[ndims - 1]
        ys = sparse_data[ndims - 2]
        if len(sparse_data) > ndims:
            if weights.count("size") > 0:
                params["s"] = sparse_data[-1] * size
            if weights.count("color") > 0:
                params["c"] = sparse_data[-1]
                params["cmap"] = cbar["name"]

        scat = ax.scatter(xs, ys, **params)
        if len(sparse_data) > ndims and weights.count("color") > 0:
            c = plt.colorbar(scat, ax=ax, cax=cax)
            c.ax.set_ylabel(axis_label(name="Weights", log=cbar["log"]))
            out["cb"] = c

        ax.set_xlabel(axis_label(coords[sparse_dim]))
        if ndims > 1:
            ax.set_ylabel(axis_label(coords[dims[int(ndims == 3)]]))

        ax.legend()
        if title is not None:
            ax.set_title(title)
        if logx or logxy:
            ax.set_xscale("log")
        if logy or logxy:
            ax.set_yscale("log")

    elif ndims == 3:

        fig = ipv.figure(width=config.width, height=config.height,
                         animation=0)

        params = dict(color=color)
        params["x"] = sparse_data[ndims - 1]
        params["y"] = sparse_data[ndims - 2]
        params["z"] = sparse_data[0]
        if len(sparse_data) > ndims and weights:
            scalar_map = cm.ScalarMappable(norm=norm,
                                                         cmap=self.cb["name"])
            params["color"] = sparse_data[-1]
            params["cmap"] = cbar["name"]

        scat = ax.scatter(xs, ys, **params)
        if len(sparse_data) > ndims and weights.count("color") > 0:
            c = plt.colorbar(scat, ax=ax, cax=cax)
            c.ax.set_ylabel(axis_label(name="Weights", log=cbar["log"]))
            out["cb"] = c

        ax.set_xlabel(axis_label(coords[sparse_dim]))
        if ndims > 1:
            ax.set_ylabel(axis_label(coords[dims[int(ndims == 3)]]))
        if ndims == 3:
            ax.set_zlabel(axis_label(coords[dims[0]]))

        ax.legend()
        if title is not None:
            ax.set_title(title)
        if logx or logxy:
            ax.set_xscale("log")
        if logy or logxy:
            ax.set_yscale("log")

    else:
        raise RuntimeError("Scatter plots for sparse data support at most "
                           "3 dimensions.")



    params = dict(label=name, edgecolors="#ffffff", c=color)
    xs = sparse_data[ndims - 1]
    ys = sparse_data[ndims - 2]
    if ndims == 3:
        params["zs"] = sparse_data[0]
    if len(sparse_data) > ndims:
        if weights.count("size") > 0:
            params["s"] = sparse_data[-1] * size
        if weights.count("color") > 0:
            params["c"] = sparse_data[-1]
            params["cmap"] = cbar["name"]

    scat = ax.scatter(xs, ys, **params)
    if len(sparse_data) > ndims and weights.count("color") > 0:
        c = plt.colorbar(scat, ax=ax, cax=cax)
        c.ax.set_ylabel(axis_label(name="Weights", log=cbar["log"]))
        out["cb"] = c

    ax.set_xlabel(axis_label(coords[sparse_dim]))
    if ndims > 1:
        ax.set_ylabel(axis_label(coords[dims[int(ndims == 3)]]))
    if ndims == 3:
        ax.set_zlabel(axis_label(coords[dims[0]]))

    ax.legend()
    if title is not None:
        ax.set_title(title)
    if logx or logxy:
        ax.set_xscale("log")
    if logy or logxy:
        ax.set_yscale("log")





    data = dict(type=plot_type, mode='markers', name=name,
                marker={"line": {"color": '#ffffff',
                                 "width": 1},
                        "opacity": opacity,
                        "color": color[0]})

    for i in range(ndims):
        data[xyz[i]] = sparse_data[ndims - 1 - i]
    if len(sparse_data) > ndims:

        # TODO: Having both color and size code for the weights leads to
        # strange behaviour in plotly, where it would seem some objects are
        # either modified or run out of scope. We will keep just the color
        # for now.

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
        showlegend=True,
        legend=dict(x=0.0, y=1.15, orientation="h"),
        height=config.height
    )
    if ndims > 1:
        layout["yaxis"] = dict(title=axis_label(coords[dims[0]]))
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
