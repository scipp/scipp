# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from plotly.colors import DEFAULT_PLOTLY_COLORS
from .plot_1d import plot_1d
from .plot_2d import plot_2d
from .plot_3d import plot_3d


def plot_plotly(input_data, ndim=0, name=None,
                collapse=None, projection="2d", **kwargs):
    """
    Function to automatically dispatch the input dataset to the appropriate
    plotting function depending on its dimensions
    """

    if ndim == 1:
        plot_1d(input_data, **kwargs)
    elif projection.lower() == "2d":
        plot_2d(input_data, name=name, ndim=ndim, **kwargs)
    elif projection.lower() == "3d":
        plot_3d(input_data, name=name, ndim=ndim, **kwargs)
    else:
        raise RuntimeError("Wrong projection type.")
    return


def get_plotly_color(index=0):
    return DEFAULT_PLOTLY_COLORS[index % len(DEFAULT_PLOTLY_COLORS)]
