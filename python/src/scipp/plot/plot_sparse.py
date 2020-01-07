# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .render import render_plot
from .sparse import visit_sparse_data
from .tools import parse_params
from ..utils import name_with_unit

# Other imports
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import cm


def plot_sparse(scipp_obj_dict, output=None, ndim=0, sparse_dim=None,
                logx=False, logy=False, logxy=False, weights="color",
                size=10.0, filename=None, axes=None, mpl_axes=None,
                opacity=0.7, title=None, mpl_scatter_params=None, cmap=None,
                log=None, vmin=None, vmax=None):
    """
    Produce a scatter plot from sparse data.
    TODO: make plot_sparse use the slicer machinery to also have buttons and
    sliders.
    """

    sparse_data = {}
    data_minmax = []
    key_save = None
    key_weights = None

    for key, data_array in scipp_obj_dict.items():

        key_save = key

        xmin, xmax, data, dims, ndims = visit_sparse_data(
            data_array, sparse_dim=sparse_dim, return_sparse_data=True,
            weights=weights)

        sparse_data[key] = {"data": data, "dims": dims, "ndims": ndims,
                            "coords": data_array.coords,
                            "has_weights": len(data) > ndims}

        if sparse_data[key]["has_weights"]:
            key_weights = key
            data_minmax.append(np.amin(data[-1]))
            data_minmax.append(np.amax(data[-1]))

    # Parse parameters for colorbar
    if key_weights is not None:
        globs = {"cmap": cmap, "log": log, "vmin": vmin, "vmax": vmax}
        cbar = parse_params(globs=globs, array=data_minmax)

    members = {"scatter": {}}
    ipv = None
    fig = None
    widg = None

    if ndims < 3:
        # fig = None
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

        # widg = None
        members.update(ax)

        for i, (key, data_array) in enumerate(scipp_obj_dict.items()):
            params = dict(label=data_array.name, edgecolors="#ffffff",
                          c=mpl_scatter_params["color"][i],
                          marker=mpl_scatter_params["marker"][i])
            xs = sparse_data[key]["data"][sparse_data[key]["ndims"] - 1]
            ys = sparse_data[key]["data"][sparse_data[key]["ndims"] - 2]
            if sparse_data[key]["has_weights"]:
                if weights.count("size") > 0:
                    params["s"] = sparse_data[key]["data"][-1] * size
                    params["alpha"] = opacity
                if weights.count("color") > 0:
                    params["c"] = sparse_data[key]["data"][-1]
                    params["cmap"] = cbar["cmap"]
                    params["norm"] = cbar["norm"]

            # print(params["norm"])
            members["scatter"][key] = ax["ax"].scatter(xs, ys, **params)

        if key_weights is not None and weights.count("color") > 0:
            colorbar = plt.colorbar(members["scatter"][key_weights],
                                    ax=ax["ax"], cax=ax["cax"])
            colorbar.ax.set_ylabel(name_with_unit(name="Weights",
                                                  log=cbar["log"]))
            members["colorbars"] = colorbar

        ax["ax"].set_xlabel(
            name_with_unit(sparse_data[key_save]["coords"][sparse_dim]))
        if ndims > 1:
            ax["ax"].set_ylabel(
                name_with_unit(
                    sparse_data[key_save]["coords"]
                    [sparse_data[key_save]["dims"][0]]))

        ax["ax"].legend()
        if title is not None:
            ax["ax"].set_title(title)
        if logx or logxy:
            ax["ax"].set_xscale("log")
        if logy or logxy:
            ax["ax"].set_yscale("log")

    elif ndims == 3:

        import ipyvolume as ipv

        widg = ipv.figure(width=config.width, height=config.height,
                          animation=0)

        # Map mpl scatter markers to ipyvolume scatter markers
        ipvmarkers = ["sphere", "arrow", "box", "diamond", "point_2d",
                      "square_2d", "triangle_2d", "circle_2d"]
        markers = {}
        for i, m in enumerate(config.marker):
            if i < len(ipvmarkers):
                markers[m] = ipvmarkers[i]
            else:
                markers[m] = np.random.choice(ipvmarkers)

        if key_weights is not None:
            scalar_map = cm.ScalarMappable(norm=cbar["norm"],
                                           cmap=cbar["cmap"])

        for i, (key, data_array) in enumerate(scipp_obj_dict.items()):
            params = dict(color=mpl_scatter_params["color"][i],
                          marker=markers[mpl_scatter_params["marker"][i]])
            if sparse_data[key]["has_weights"]:
                params["color"] = scalar_map.to_rgba(
                    sparse_data[key]["data"][-1])

            members["scatter"][key] = ipv.scatter(
                x=sparse_data[key]["data"][sparse_data[key]["ndims"] - 1],
                y=sparse_data[key]["data"][sparse_data[key]["ndims"] - 2],
                z=sparse_data[key]["data"][0], **params)

        widg.xlabel = name_with_unit(sparse_data[key_save]["coords"]
                                     [sparse_dim])
        widg.ylabel = name_with_unit(sparse_data[key_save]["coords"]
                                     [sparse_data[key_save]["dims"][1]])
        widg.zlabel = name_with_unit(sparse_data[key_save]["coords"]
                                     [sparse_data[key_save]["dims"][0]])

        # widg = fig

    else:
        raise RuntimeError("Scatter plots for sparse data support at most "
                           "3 dimensions.")

    render_plot(figure=fig, widgets=widg, filename=filename, ipv=ipv,
                output=output)

    members.update({"fig": fig, "widgets": widg})

    return members
