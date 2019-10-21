# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

# Scipp imports
from . import config
from .tools import edges_to_centers, centers_to_edges, axis_label, \
                   parse_colorbar, get_1d_axes, axis_to_dim_label

# Other imports
import numpy as np
import matplotlib.pyplot as plt


def plot_1d(input_data, logx=False, logy=False, logxy=False,
            color=None, filename=None, title=None, axes=None, mpl_axes=None):
    """
    Plot a 1D spectrum.
    Input is a dictionary containing a list of DataProxy.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    """

    fig = None
    if mpl_axes is not None:
        ax = mpl_axes
    else:
        fig, ax = plt.subplots(1, 1)

    out = {"fill_between": {}, "step": {}, "line": {},
           "errorbar": {}}

    for i, (name, var) in enumerate(input_data.items()):

        xlab, ylab, x, y = get_1d_axes(var, axes, name)

        # Check for bin edges
        if x.shape[0] == y.shape[0] + 1:
            xe = x.copy()
            ye = np.concatenate(([0], y))
            x = edges_to_centers(x)
            out["fill_between"][name] = ax.fill_between(
                xe, ye, step="pre", alpha=0.6, label=ylab, color=color[i])
            out["step"][name] = ax.step(xe, ye, color=color[i])
        else:
            out["line"][name] = ax.plot(x, y, label=ylab, color=color[i])
        # Include variance if present
        if var.variances is not None:
            out["errorbar"][name] = ax.errorbar(x, y,
                                                yerr=np.sqrt(var.variances),
                                                linestyle='None',
                                                ecolor=color[i])

    ax.set_xlabel(xlab)
    ax.legend()
    if title is not None:
        ax.set_title(title)
    if logx or logxy:
        ax.set_xscale("log")
    if logy or logxy:
        ax.set_yscale("log")

    out["ax"] = ax
    if fig is not None:
        out["fig"] = fig

    return out


def plot_2d(input_data, name=None, axes=None, contours=False, cb=None,
            filename=None, variances=False, mpl_axes=None, mpl_cax=None,
            **kwargs):
    """
    Plot a 2D image.
    If countours=True, a filled contour plot is produced, if False, then a
    standard image made of pixels is created.
    """

    if input_data.variances is None and variances:
        raise RuntimeError("The supplied data does not contain variances.")

    if axes is None:
        axes = input_data.dims

    # Get coordinates axes and dimensions
    coords = input_data.coords
    zdims = input_data.dims
    nz = input_data.shape

    dimx, labx, xcoord = axis_to_dim_label(input_data, axes[-1])
    dimy, laby, ycoord = axis_to_dim_label(input_data, axes[-2])
    xy = [xcoord.values, ycoord.values]

    # Check for bin edges
    dims = [xcoord.dims[0], ycoord.dims[0]]
    shapes = [xcoord.shape[0], ycoord.shape[0]]
    # Find the dimension in z that corresponds to x and y
    transpose = (zdims[0] == dims[0]) and (zdims[1] == dims[1])
    idx = np.roll([1, 0], int(transpose))

    grid_edges = [None, None]
    grid_centers = [None, None]
    for i in range(2):
        if shapes[i] == nz[idx[i]]:
            grid_edges[i] = centers_to_edges(xy[i])
            grid_centers[i] = xy[i]
        elif shapes[i] == nz[idx[i]] + 1:
            grid_edges[i] = xy[i]
            grid_centers[i] = edges_to_centers(xy[i])
        else:
            raise RuntimeError("Dimensions of x Coord ({}) and Value ({}) do "
                               "not match.".format(shapes[i], nz[idx[i]]))

    # Parse colorbar
    cbar = parse_colorbar(config.cb, cb)

    # Get or create matplotlib axes
    fig = None
    if mpl_axes is not None:
        ax = mpl_axes
    else:
        fig, ax = plt.subplots(1, 1)

    ax.set_xlabel(axis_label(xcoord, name=labx))
    ax.set_ylabel(axis_label(ycoord, name=laby))

    if variances:
        z = input_data.variances
        cblab = "Variances"
    else:
        z = input_data.values
        cblab = name
    # Transpose the data?
    if transpose:
        z = z.T
    # Apply colorbar parameters
    if cbar["log"]:
        with np.errstate(invalid="ignore", divide="ignore"):
            z = np.log10(z)
    if cbar["min"] is not None:
        vmin = cbar["min"]
    else:
        vmin = np.amin(z[np.where(np.isfinite(z))])
    if cbar["max"] is not None:
        vmax = cbar["max"]
    else:
        vmax = np.amax(z[np.where(np.isfinite(z))])

    args = {"vmin": vmin, "vmax": vmax, "cmap": cbar["name"]}
    if contours:
        img = ax.contourf(grid_centers[0], grid_centers[1], z, **args)
    else:
        img = ax.imshow(z, extent=[grid_edges[0][0], grid_edges[0][-1],
                                   grid_edges[1][0], grid_edges[0][-1]],
                        origin="lower", aspect="auto", **args)
    cb = plt.colorbar(img, ax=ax, cax=mpl_cax)
    cb.ax.set_ylabel(axis_label(var=input_data, name=cblab, log=cbar["log"]))

    out = {"ax": ax, "cb": cb, "img": img}
    if fig is not None:
        out["fig"] = fig

    return out
