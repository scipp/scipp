# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet


def render_plot(figure=None, widgets=None, filename=None):
    """
    Render the plot using either file export or interactive display.
    """

    # Delay imports
    import IPython.display as disp

    if filename is not None:
        figure.savefig(filename)
    else:
        disp.display(widgets)
    return
