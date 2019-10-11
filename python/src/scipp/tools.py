# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc


def edges_to_centers(x):
    """
    Convert coordinate edges to centers, and return also the widths
    """
    return 0.5 * (x[1:] + x[:-1]), np.ediff1d(x)


def centers_to_edges(x):
    """
    Convert coordinate centers to edges
    """
    e = edges_to_centers(x)[0]
    return np.concatenate([[2.0 * x[0] - e[0]], e, [2.0 * x[-1] - e[-1]]])


def axis_label(var, name=None, log=False, replace_dim=True):
    """
    Make an axis label with "Name [unit]"
    """
    if name is not None:
        label = name
    else:
        label = str(var.dims[0])
        if replace_dim:
            label = label.replace("Dim.", "")

    if log:
        label = "log\u2081\u2080(" + label + ")"
    if var.unit != sc.units.dimensionless:
        label += " [{}]".format(var.unit)
    return label


def parse_colorbar(default, cb, plotly=False):
    """
    Construct the colorbar using default and input values
    """
    cbar = default.copy()
    if cb is not None:
        for key, val in cb.items():
            cbar[key] = val
    # In plotly, colorbar names start with an uppercase letter
    if plotly:
        cbar["name"] = "{}{}".format(cbar["name"][0].upper(), cbar["name"][1:])
    return cbar


def get_coord_array(coords, labels, axis):
    name = None
    if isinstance(axis, sc.Dim):
        var = coords[axis]
    elif isinstance(axis, str):
        var = labels[axis]
        name = axis
    else:
        var = axis
    return axis_label(var, name=name), var


def process_dimensions(input_data, coords, labels, axes):
    """
    Make x and y arrays from dimensions and check for bins edges
    """
    zlabs = input_data.dims
    nz = input_data.shape

    if axes is None:
        axes = input_data.dims
    else:
        axes = axes[-2:]

    xcoord_name, xcoord = get_coord_array(coords, labels, axes[-1])
    ycoord_name, ycoord = get_coord_array(coords, labels, axes[-2])
    x = xcoord.values
    y = ycoord.values

    # Check for bin edges
    # TODO: find a better way to handle edges. Currently, we convert from
    # edges to centers and then back to edges afterwards inside the plotly
    # object. This is not optimal and could lead to precision loss issues.
    ylabs = ycoord.dims
    ny = ycoord.shape
    xlabs = xcoord.dims
    nx = xcoord.shape
    # Find the dimension in z that corresponds to x and y
    ix = iy = None
    for i in range(len(zlabs)):
        if zlabs[i] == xlabs[0]:
            ix = i
        if zlabs[i] == ylabs[0]:
            iy = i
    if (ix is None) or (iy is None):
        raise RuntimeError(
            "Dimension of either x ({}) or y ({}) array was not "
            "found in z ({}) array.".format(
                xlabs, ylabs, zlabs))
    if nx[0] == nz[ix]:
        xe = centers_to_edges(x)
        xc = x
    elif nx[0] == nz[ix] + 1:
        xe = x
        xc = edges_to_centers(x)[0]
    else:
        raise RuntimeError("Dimensions of x Coord ({}) and Value ({}) do not "
                           "match.".format(nx[0], nz[ix]))
    if ny[0] == nz[iy]:
        ye = centers_to_edges(y)
        yc = y
    elif ny[0] == nz[iy] + 1:
        ye = y
        yc = edges_to_centers(y)[0]
    else:
        raise RuntimeError("Dimensions of y Coord ({}) and Value ({}) do not "
                           "match.".format(ny[0], nz[iy]))
    return xcoord_name, ycoord_name, xe, ye, xc, yc, xlabs, ylabs, zlabs
