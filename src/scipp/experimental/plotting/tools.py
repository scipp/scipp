# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ... import config, scalar, concat, midpoints

from copy import copy
from matplotlib.colors import LinearSegmentedColormap
from matplotlib import cm
import matplotlib.pyplot as plt
from typing import Any
from io import BytesIO


def get_cmap(name: str):

    if name is None:
        name = config['plot']['params']['cmap']

    try:
        cmap = copy(cm.get_cmap(name))
    except ValueError:
        cmap = LinearSegmentedColormap.from_list("tmp", [name, name])

    cmap.set_under(config['plot']['params']["under_color"])
    cmap.set_over(config['plot']['params']["over_color"])
    return cmap


def fig_to_pngbytes(fig: Any):
    """
    Convert figure to png image bytes.
    We also close the figure to prevent it from showing up again in
    cells further down the notebook.
    """
    buf = BytesIO()
    fig.savefig(buf, format='png')
    plt.close(fig)
    buf.seek(0)
    return buf.getvalue()


def to_bin_edges(x, dim):
    """
    Convert array centers to edges
    """
    idim = x.dims.index(dim)
    if x.shape[idim] < 2:
        one = scalar(1.0, unit=x.unit)
        return concat([x[dim, 0:1] - one, x[dim, 0:1] + one], dim)
    else:
        center = midpoints(x, dim=dim)
        # Note: use range of 0:1 to keep dimension dim in the slice to avoid
        # switching round dimension order in concatenate step.
        left = center[dim, 0:1] - (x[dim, 1] - x[dim, 0])
        right = center[dim, -1] + (x[dim, -1] - x[dim, -2])
        return concat([left, center, right], dim)


def number_to_variable(x):
    """
    Convert the input int or float to a variable.
    """
    return scalar(x, unit=None) if isinstance(x, (int, float)) else x
