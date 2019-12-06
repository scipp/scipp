# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .render import render_plot
from .sparse import visit_sparse_data
from .tools import axis_label, parse_colorbar

# Other imports
import matplotlib.pyplot as plt
from matplotlib import cm
# try:
#     import ipyvolume as ipv
# except:
#     pass


def plot_sparse(input_data, ndim=0, sparse_dim=None, logx=False,
                logy=False, logxy=False, weights="color", size=10.0,
                filename=None, axes=None, cb=None, opacity=0.7, color=None,
                title=None, marker=None):
    """
    Produce a scatter plot from sparse data.
    TODO: make plot_sparse use the slicer machinery to also have buttons and
    sliders.
    """

    xmin, xmax, sparse_data, var, name, dims, ndims = visit_sparse_data(
        input_data, sparse_dim=sparse_dim, return_sparse_data=True,
        weights=weights)

    coords = var.coords

    # Parse colorbar
    cbar = parse_colorbar(cb, var, values=sparse_data[-1])

    members = {}
    ipv = None


    if ndims < 3:
        fig, ax = plt.subplots(1, 1, figsize=(config.width/config.dpi,
                                              config.height/config.dpi),
                               dpi=config.dpi)
        widg = None
        members["ax"] = ax
        params = dict(label=name, edgecolors="#ffffff", c=color)
        xs = sparse_data[ndims - 1]
        ys = sparse_data[ndims - 2]
        if len(sparse_data) > ndims:
            if weights.count("size") > 0:
                params["s"] = sparse_data[-1] * size
                params["alpha"] = opacity
            if weights.count("color") > 0:
                params["c"] = sparse_data[-1]
                params["cmap"] = cbar["name"]
                params["norm"] = cbar["norm"]["values"]

        if marker is None:
            marker = "o"
        scat = ax.scatter(xs, ys, **params)
        if len(sparse_data) > ndims and weights.count("color") > 0:
            colorbar = plt.colorbar(scat, ax=ax, cax=None)
            colorbar.ax.set_ylabel(axis_label(name="Weights", log=cbar["log"]))
            members["colorbars"] = colorbar

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

        import ipyvolume as ipv

        fig = ipv.figure(width=config.width, height=config.height,
                         animation=0)

        params = dict(color=color)
        params["x"] = sparse_data[ndims - 1]
        params["y"] = sparse_data[ndims - 2]
        params["z"] = sparse_data[0]
        if len(sparse_data) > ndims and weights:
            scalar_map = cm.ScalarMappable(norm=cbar["norm"]["values"],
                                           cmap=cbar["name"])
            params["color"] = scalar_map.to_rgba(sparse_data[-1])

        if marker is None:
            marker = "sphere"
        params["marker"] = marker
        scat = ipv.scatter(**params)

        fig.xlabel = axis_label(coords[sparse_dim])
        fig.ylabel = axis_label(coords[dims[int(ndims == 3)]])
        fig.zlabel = axis_label(coords[dims[0]])

        # if logx or logxy:
        #     ax.set_xscale("log")
        # if logy or logxy:
        #     ax.set_yscale("log")
        widg = fig


    else:
        raise RuntimeError("Scatter plots for sparse data support at most "
                           "3 dimensions.")

    render_plot(figure=fig, widgets=widg, filename=filename, ipv=ipv)

    members.update({"fig": fig, "scatter": scat})

    return members
