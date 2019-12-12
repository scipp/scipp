# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .._scipp.core import Dim
from .._scipp.core.units import dimensionless

# Other imports
import numpy as np
from matplotlib.colors import Normalize, LogNorm, LinearSegmentedColormap


def get_line_param(name=None, index=None):
    """
    Get the default line parameter from the config.
    If an index is supplied, return the i-th item in the list.
    """
    param = getattr(config, name)
    return param[index % len(param)]


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


def parse_params(params=None, defaults=None, globs=None, array=None):
    """
    Construct the colorbar settings using default and input values
    """
    parsed = config.params.copy()
    if defaults is not None:
        for key, val in defaults.items():
            parsed[key] = val
    if globs is not None:
        for key, val in globs.items():
            # Global parameters need special treatment because by default they
            # are set to None, and we don't want to overwrite the defaults.
            if val is not None:
                parsed[key] = val
    if params is not None:
        if isinstance(params, bool):
            params = {"show": params}
        for key, val in params.items():
            parsed[key] = val

    if array is not None:
        if parsed["log"]:
            with np.errstate(divide="ignore", invalid="ignore"):
                subset = np.where(np.isfinite(np.log10(array)))
        else:
            subset = np.where(np.isfinite(array))
        if parsed["vmin"] is not None:
            vmin = parsed["vmin"]
        else:
            vmin = np.amin(array[subset])
        if parsed["vmax"] is not None:
            vmax = parsed["vmax"]
        else:
            vmax = np.amax(array[subset])
        if parsed["log"]:
            norm = LogNorm(vmin=vmin, vmax=vmax)
        else:
            norm = Normalize(vmin=vmin, vmax=vmax)
        parsed["norm"] = norm

    # Convert color into custom colormap
    if parsed["color"] is not None:
        parsed["cmap"] = LinearSegmentedColormap.from_list(
            "tmp", [parsed["color"], parsed["color"]])

    return parsed


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
