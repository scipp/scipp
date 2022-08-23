# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ... import config

from copy import copy
from matplotlib.colors import LinearSegmentedColormap
from matplotlib import cm
import matplotlib.pyplot as plt
from typing import Any
from io import BytesIO


def get_line_param(name: str, index: int) -> Any:
    """
    Get the default line parameter from the config.
    If an index is supplied, return the i-th item in the list.
    """
    param = config['plot'][name]
    return param[index % len(param)]


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
