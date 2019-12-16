# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .render import render_plot
from .sparse import visit_sparse_data
from .tools import axis_label, parse_params

# Other imports
import matplotlib.pyplot as plt
from matplotlib import cm


def plot_sparse(data_array, ndim=0, sparse_dim=None, logx=False,
                logy=False, logxy=False, weights="color", size=10.0,
                filename=None, axes=None, mpl_axes=None, opacity=0.7,
                title=None, mpl_scatter_params=None, cmap=None, log=None,
                vmin=None, vmax=None):
    """
    Produce a scatter plot from sparse data.
    TODO: make plot_sparse use the slicer machinery to also have buttons and
    sliders.
    """

    xmin, xmax, sparse_data, dims, ndims = visit_sparse_data(
        data_array, sparse_dim=sparse_dim, return_sparse_data=True,
        weights=weights)

    coords = data_array.coords

    # Parse parameters for colorbar
    globs = {"cmap": cmap, "log": log, "vmin": vmin, "vmax": vmax}
    cbar = parse_params(globs=globs, array=sparse_data[-1])

    members = {}
    ipv = None

    if ndims < 3:
        fig = None
        ax = {"ax": None, "cax": None}
        if mpl_axes is not None:
            if isinstance(mpl_axes, dict):
                ax.update(mpl_axes)
            else:
                # Case where only a single axis is given
                ax["ax"] = mpl_axes
        else:
            fig, ax["ax"] = plt.subplots(
                1, 1, figsize=(config.width/config.dpi,
                               config.height/config.dpi),
                dpi=config.dpi)

        widg = None
        members.update(ax)
        params = dict(label=data_array.name, edgecolors="#ffffff",
                      c=mpl_scatter_params["color"][0],
                      marker=mpl_scatter_params["marker"][0])
        xs = sparse_data[ndims - 1]
        ys = sparse_data[ndims - 2]
        if len(sparse_data) > ndims:
            if weights.count("size") > 0:
                params["s"] = sparse_data[-1] * size
                params["alpha"] = opacity
            if weights.count("color") > 0:
                params["c"] = sparse_data[-1]
                params["cmap"] = cbar["cmap"]
                params["norm"] = cbar["norm"]

        scat = ax["ax"].scatter(xs, ys, **params)
        if len(sparse_data) > ndims and weights.count("color") > 0:
            colorbar = plt.colorbar(scat, ax=ax["ax"], cax=ax["cax"])
            colorbar.ax.set_ylabel(axis_label(name="Weights", log=cbar["log"]))
            members["colorbars"] = colorbar

        ax["ax"].set_xlabel(axis_label(coords[sparse_dim]))
        if ndims > 1:
            ax["ax"].set_ylabel(axis_label(coords[dims[int(ndims == 3)]]))

        ax["ax"].legend()
        if title is not None:
            ax["ax"].set_title(title)
        if logx or logxy:
            ax["ax"].set_xscale("log")
        if logy or logxy:
            ax["ax"].set_yscale("log")

    elif ndims == 3:

        import ipyvolume as ipv

        fig = ipv.figure(width=config.width, height=config.height,
                         animation=0)

        params = dict(color=mpl_scatter_params["color"][0],
                      marker=mpl_scatter_params["marker"][0])
        if len(sparse_data) > ndims and weights:
            scalar_map = cm.ScalarMappable(norm=cbar["norm"],
                                           cmap=cbar["cmap"])
            params["color"] = scalar_map.to_rgba(sparse_data[-1])

        if params["marker"] is None or len(params["marker"]) == 1:
            params["marker"] = "sphere"
        scat = ipv.scatter(x=sparse_data[ndims - 1], y=sparse_data[ndims - 2],
                           z=sparse_data[0], **params)

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
