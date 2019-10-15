# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from . import config

# Other imports
import IPython.display as disp
from plotly.io import write_html, write_image, to_image


def get_color(index=0):
    """
    Get the next color in the list of standard colors.
    """
    from plotly.colors import DEFAULT_PLOTLY_COLORS
    return DEFAULT_PLOTLY_COLORS[index % len(DEFAULT_PLOTLY_COLORS)]


def render_plot(static_fig=None, interactive_fig=None, backend=None,
                filename=None):
    """
    Render the plot using either file export, static png inline display or
    interactive display.
    """

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
