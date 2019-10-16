# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from . import config
from .._scipp.core.units import dimensionless

# Other imports
import numpy as np


def render_plot(static_fig=None, interactive_fig=None, backend=None,
                filename=None):
    """
    Render the plot using either file export, static png inline display or
    interactive display.
    """

    # Delay imports
    import IPython.display as disp
    from plotly.io import write_html, write_image, to_image

    if backend is None:
        backend = config.backend

    if filename is not None:
        if filename.endswith(".html"):
            write_html(fig=static_fig, file=filename, auto_open=False)
        else:
            write_image(fig=static_fig, file=filename)
    else:
        if backend == "static":
            disp.display(disp.Image(to_image(static_fig, format='png')))
        elif backend == "interactive":
            disp.display(interactive_fig)
        else:
            raise RuntimeError("Unknown backend {}. Currently supported "
                               "backends are 'interactive' and "
                               "'static'".format(backend))
    return


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
    if var.unit != dimensionless:
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
        cbar["name"] = cbar["name"].capitalize()
    return cbar
