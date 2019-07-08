# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scippy as sp

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


def axis_label(var, name=None, log=False):
    """
    Make an axis label with "Name [unit]"
    """
    if name is not None:
        label = name
    else:
        label = str(var.dims.labels[0]).replace("Dim.", "")

    if log:
        label = "log\u2081\u2080(" + label + ")"
    if var.unit != sp.units.dimensionless:
        label += " [{}]".format(var.unit)
    return label