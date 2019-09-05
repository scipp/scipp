# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import matplotlib.pyplot as plt
from .tools import edges_to_centers, axis_label, parse_colorbar, \
                   process_dimensions


def plot_matplotlib(input_data, ndim=0, name=None, config=None, **kwargs):
    """
    Function to automatically dispatch the input dataset to the appropriate
    plotting function depending on its dimensions
    """

    if ndim == 1:
        plot_1d(input_data, **kwargs)
    elif ndim == 2:
        plot_2d(input_data, name=name, config=config, **kwargs)
    elif ndim > 2:
        raise RuntimeError("Plotting for 3 and more dimensions in matplotlib "
                           "is not yet implemented. Try using the plotly "
                           "backend instead by setting "
                           "scipp.plot_config.backend = 'plotly'.")
    else:
        raise RuntimeError("Invalid number of dimensions for "
                           "plotting: {}".format(ndim))
    return


def plot_1d(input_data, logx=False, logy=False, logxy=False, axes=None,
            color=None, filename=None, title=None):
    """
    Plot a 1D spectrum.

    Input is a dictionary containing a list of DataProxy.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.

    TODO: find a more general way of handling arguments to be sent to plotly,
    probably via a dictionay of arguments
    """

    fig = plt.figure()
    ax = fig.add_subplot(111)

    color_count = 0
    for name, var in input_data.items():
        xcoord = var.coords[var.dims[0]]
        x = xcoord.values
        xlab = axis_label(xcoord)
        y = var.values
        ylab = axis_label(var=var, name=name)

        # Check for bin edges
        if x.shape[0] == y.shape[0] + 1:
            xe = x.copy()
            ye = np.concatenate(([0], y))
            x, w = edges_to_centers(x)
            if var.variances is not None:
                yerr = np.sqrt(var.variances)
            else:
                yerr = None
            ax.bar(x, y, width=w, yerr=yerr, label=ylab, alpha=0.6,
                   color=color[color_count], ecolor=color[color_count])
            ax.step(xe, ye, color=color[color_count])
            ax.plot([xe[-1], xe[-1]], [ye[-1], 0], color=color[color_count])
        else:
            # Include variance if present
            if var.variances is not None:
                ax.errorbar(x, y, yerr=np.sqrt(var.variances),
                            label=ylab, color=color[color_count],
                            ecolor=color[color_count])
            else:
                ax.plot(x, y, label=ylab, color=color[color_count])
        color_count += 1

    ax.set_xlabel(xlab)
    ax.legend()
    if title is not None:
        ax.set_title(title)
    if filename is not None:
        fig.savefig(filename, bbox_inches="tight")

    return


def plot_2d(input_data, name=None, axes=None, contours=False, cb=None,
            filename=None, show_variances=False, config=None, **kwargs):
    """
    Plot a 2D image.

    If countours=True, a filled contour plot is produced, if False, then a
    standard image made of pixels is created.
    """

    if axes is None:
        axes = input_data.dims

    # Get coordinates axes and dimensions
    coords = input_data.coords
    labels = input_data.labels
    xcoord, ycoord, xe, ye, xc, yc, xlabs, ylabs, zlabs = \
        process_dimensions(input_data=input_data, coords=coords,
                           labels=labels, axes=axes)

    # Parse colorbar
    cbar = parse_colorbar(config.cb, cb)

    # Prepare dictionary for holding key parameters
    data = {"values": {"cbmin": "min", "cbmax": "max", "name": name}}

    if input_data.variances is not None and show_variances:
        fig, ax = plt.subplots(1, 2, sharex=True, sharey=True)
        fig_size = fig.get_size_inches()
        fig.set_size_inches(fig_size[0]*2, fig_size[1])
        # Append parameters to data dictionary
        data["variances"] = {"cbmin": "min_var", "cbmax": "max_var",
                             "name": "variances"}
    else:
        fig, ax = plt.subplots(1, 1)
        ax = [ax]

    for i, (key, val) in enumerate(sorted(data.items())):

        ax[i].set_xlabel(xcoord)
        ax[i].set_ylabel(ycoord)

        z = getattr(input_data, key)
        # Check if dimensions of arrays agree, if not, plot the transpose
        if (zlabs[0] == xlabs[0]) and (zlabs[1] == ylabs[0]):
            z = z.T
        # Apply colorbar parameters
        if cbar["log"]:
            with np.errstate(invalid="ignore", divide="ignore"):
                z = np.log10(z)
        if cbar[val["cbmin"]] is None:
            cbar[val["cbmin"]] = np.amin(z[np.where(np.isfinite(z))])
        if cbar[val["cbmax"]] is None:
            cbar[val["cbmax"]] = np.amax(z[np.where(np.isfinite(z))])

        args = {"vmin": cbar[val["cbmin"]], "vmax": cbar[val["cbmax"]],
                "cmap": cbar["name"]}
        if contours:
            img = ax[i].contourf(xc, yc, z, **args)
        else:
            img = ax[i].imshow(z, extent=[xe[0], xe[-1], ye[0], ye[-1]],
                               origin="lower", aspect="auto", **args)
        cb = plt.colorbar(img, ax=ax[i])
        cb.ax.set_ylabel(axis_label(var=input_data, name=val["name"],
                         log=cbar["log"]))
        cb.ax.yaxis.set_label_coords(-1.1, 0.5)

    if filename is not None:
        fig.savefig(filename, bbox_inches="tight")

    return


def get_mpl_color(index=0):
    prop_cycle = plt.rcParams["axes.prop_cycle"]
    colors = prop_cycle.by_key()["color"]
    return colors[index % len(colors)]
