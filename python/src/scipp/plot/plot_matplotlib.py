# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

# Scipp imports
from .sparse import visit_sparse_data
from .tools import edges_to_centers, centers_to_edges, axis_label, \
                   parse_colorbar, axis_to_dim_label

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

    if axes is None:
        axes = input_data.dims

    for i, (name, var) in enumerate(input_data):

        dim, lab, xcoord = axis_to_dim_label(var, axes[-1])
        x = xcoord.values
        xlab = axis_label(var=xcoord, name=lab)
        y = var.values
        ylab = axis_label(var=var, name="")

        # Check for bin edges
        if x.shape[0] == y.shape[0] + 1:
            xe = x.copy()
            ye = np.concatenate(([0], y))
            x = edges_to_centers(x)
            out["fill_between"][name] = ax.fill_between(
                xe, ye, step="pre", alpha=0.6, label=name, color=color[i])
            out["step"][name] = ax.step(xe, ye, color=color[i])
        else:
            out["line"][name] = ax.plot(x, y, label=name, color=color[i])
        # Include variance if present
        if var.variances is not None:
            out["errorbar"][name] = ax.errorbar(x, y,
                                                yerr=np.sqrt(var.variances),
                                                linestyle='None',
                                                ecolor=color[i])

    ax.set_xlabel(xlab)
    ax.set_ylabel(ylab)
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
            filename=None, show_variances=False, mpl_axes=None, mpl_cax=None):
    """
    Plot a 2D image.
    If countours=True, a filled contour plot is produced, if False, then a
    standard image made of pixels is created.
    """

    var = input_data[name]

    if var.variances is None and show_variances:
        raise RuntimeError("The supplied data does not contain variances.")

    if axes is None:
        axes = var.dims

    # Get coordinates axes and dimensions
    zdims = var.dims
    nz = var.shape

    dimx, labx, xcoord = axis_to_dim_label(var, axes[-1])
    dimy, laby, ycoord = axis_to_dim_label(var, axes[-2])
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
    cbar = parse_colorbar(cb)

    # Get or create matplotlib axes
    fig = None
    if mpl_axes is not None:
        ax = mpl_axes
    else:
        fig, ax = plt.subplots(1, 1 + show_variances)
    if mpl_cax is not None:
        cax = mpl_cax
    else:
        cax = [None] * (1 + show_variances)

    # Make sure axes are stored in arrays
    array_types = [list, np.ndarray]
    if type(ax) not in array_types:
        ax = [ax]
    if type(cax) not in array_types:
        cax = [cax]

    # Update axes labels
    for a in ax:
        a.set_xlabel(axis_label(xcoord, name=labx))
        a.set_ylabel(axis_label(ycoord, name=laby))

    params = {"values": {"cbmin": "min", "cbmax": "max", "cblab": name}}
    if show_variances:
        params["variances"] = {"cbmin": "min_var", "cbmax": "max_var",
                               "cblab": "variances"}

    out = {}

    for i, (key, param) in enumerate(sorted(params.items())):
        # if param is not None:
        arr = getattr(var, key)
        if cbar["log"]:
            with np.errstate(invalid="ignore", divide="ignore"):
                arr = np.log10(arr)
        if cbar[param["cbmin"]] is not None:
            vmin = cbar[param["cbmin"]]
        else:
            vmin = np.amin(arr[np.where(np.isfinite(arr))])
        if cbar[param["cbmax"]] is not None:
            vmax = cbar[param["cbmax"]]
        else:
            vmax = np.amax(arr[np.where(np.isfinite(arr))])

        if transpose:
            arr = arr.T

        args = {"vmin": vmin, "vmax": vmax, "cmap": cbar["name"]}
        if contours:
            img = ax[i].contourf(grid_centers[0], grid_centers[1], arr, **args)
        else:
            img = ax[i].imshow(arr, extent=[grid_edges[0][0],
                                            grid_edges[0][-1],
                                            grid_edges[1][0],
                                            grid_edges[1][-1]],
                               origin="lower", aspect="auto", **args)
        c = plt.colorbar(img, ax=ax[i], cax=cax[i])
        c.ax.set_ylabel(axis_label(var=var, name=param["cblab"],
                                   log=cbar["log"]))

        out[key] = {"ax": ax[i], "cb": c, "img": img}

    if fig is not None:
        out["fig"] = fig

    return out


def plot_sparse(input_data, ndim=0, sparse_dim=None, logx=False,
                logy=False, logxy=False, weights="color", size=10.0,
                filename=None, axes=None, cb=None, opacity=1.0,
                mpl_axes=None, mpl_cax=None, title=None, color=None):
    """
    Produce a scatter plot from sparse data.
    """

    xmin, xmax, sparse_data, var, name, dims, ndims = visit_sparse_data(
        input_data, sparse_dim=sparse_dim, return_sparse_data=True,
        weights=weights)

    if ndims > 3:
        raise RuntimeError("Scatter plots for sparse data with matplotlib "
                           "support at most 3 dimensions.")

    coords = var.coords

    # Parse colorbar
    cbar = parse_colorbar(cb)
    out = {}

    # Get or create matplotlib axes
    fig = None
    if mpl_axes is not None:
        ax = mpl_axes
    else:
        if ndims < 3:
            fig, ax = plt.subplots(1, 1)
        else:
            from mpl_toolkits.mplot3d import Axes3D # noqa
            fig, ax = plt.subplots(1, 1, subplot_kw={'projection': "3d"})
    if mpl_cax is not None:
        cax = mpl_cax
    else:
        cax = None

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

    out["ax"] = ax
    out["scat"] = scat

    if fig is not None:
        out["fig"] = fig

    return out
