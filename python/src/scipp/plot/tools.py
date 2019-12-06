# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .._scipp.core import Dim
from .._scipp.core.units import dimensionless

# Other imports
import numpy as np
from matplotlib.colors import Normalize, LogNorm


def get_color(index=0):
    """
    Get the i-th color in the list of standard colors.
    """
    return config.color_list[index % len(config.color_list)]


def edges_to_centers(x):
    """
    Convert array edges to centers
    """
    return 0.5 * (x[1:] + x[:-1])


def centers_to_edges(x):
    """
    Convert array centers to edges
    """
    e = edges_to_centers(x)
    return np.concatenate([[2.0 * x[0] - e[0]], e, [2.0 * x[-1] - e[-1]]])


def axis_label(var=None, name=None, log=False, replace_dim=True):
    """
    Make an axis label with "Name [unit]"
    """
    label = ""
    if name is not None:
        label = name
    elif var is not None:
        label = str(var.dims[-1])
        if replace_dim:
            label = label.replace("Dim.", "")

    if log:
        label = "log\u2081\u2080(" + label + ")"
    if var is not None:
        if var.unit != dimensionless:
            label += " [{}]".format(var.unit)
    return label


def parse_colorbar(cb, var=None, show_variances=False):
    """
    Construct the colorbar using default and input values
    """
    cbar = config.cb.copy()
    cbar["norm"] = dict()
    if cb is not None:
        if isinstance(cb, str):
            cbar["name"] = cb
        else:
            for key, val in cb.items():
                cbar[key] = val
    params = {"values": {"cbmin": "min", "cbmax": "max"}}
    if var.variances is not None and show_variances:
        params["variances"] = {"cbmin": "min_var", "cbmax": "max_var"}
    for key, val in sorted(params.items()):
        arr = getattr(var, key)
        if cbar["log"]:
            subset = np.where(np.isfinite(np.log10(arr)))
        else:
            subset = np.where(np.isfinite(arr))
        if cbar[val["cbmin"]] is not None:
            vmin = cbar[val["cbmin"]]
        else:
            vmin = np.amin(arr[subset])
        if cbar[val["cbmax"]] is not None:
            vmax = cbar[val["cbmax"]]
        else:
            vmax = np.amax(arr[subset])
        if cbar["log"]:
            norm = LogNorm(vmin=vmin, vmax=vmax)
        else:
            norm = Normalize(vmin=vmin, vmax=vmax)
        cbar["norm"][key] = norm
    return cbar


def axis_to_dim_label(dataset, axis):
    """
    Get dimensions and label (if present) from requested axis
    """
    if isinstance(axis, Dim):
        dim = axis
        lab = None
        var = dataset.coords[dim]
    elif isinstance(axis, str):
        # By convention, the last dim of the labels is the inner dimension,
        # but note that for now two-dimensional labels are not supported in
        # the plotting.
        dim = dataset.labels[axis].dims[-1]
        lab = axis
        var = dataset.labels[lab]
    else:
        raise RuntimeError("Unsupported axis found in 'axes': {}. This must "
                           "be either a Scipp dimension "
                           "or a string.".format(axis))
    return dim, lab, var


def get_1d_axes(var, axes, name):
    """
    Utility to simplify getting 1d axes labels and coordinate arrays
    """
    if axes is None:
        axes = {var.dims[0]: var.dims[0]}
    elif isinstance(axes, str):
        axes = {var.dims[0]: axes}
    dim, lab, xcoord = axis_to_dim_label(var, axes[var.dims[0]])
    x = xcoord.values
    xlab = axis_label(var=xcoord, name=lab)
    y = var.values
    ylab = axis_label(var=var, name="")
    return xlab, ylab, x, y, axes
