# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config

# Other imports
import matplotlib.pyplot as plt


def get_mpl_axes(ax=None, cax=None, figsize=None):

    # Matplotlib figure and axes
    fig = None
    own_axes = True
    if ax is None:
        if figsize is None:
            figsize = (config.plot.width / config.plot.dpi,
                       config.plot.height / config.plot.dpi)
        fig, ax = plt.subplots(1, 1, figsize=figsize, dpi=config.plot.dpi)
    else:
        own_axes = False
        fig = ax.get_figure()

    return fig, ax, cax, own_axes
