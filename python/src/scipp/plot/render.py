# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import matplotlib
import ipywidgets as ipw


def render_plot(figure=None, widgets=None, filename=None, output=None,
                ipv=None):
    """
    Render the plot using either file export or interactive display.
    """

    # Delay imports
    import IPython.display as disp

    if filename is not None:
        if ipv is not None:
            if filename.endswith(".html"):
                ipv.save(filename)
            else:
                raise RuntimeError("Only html export is supported for now "
                                   "when using ipyvolume.")
        else:
            figure.savefig(filename, bbox_inches="tight")
    else:
        if widgets is not None and ipv is not None or \
          matplotlib.get_backend() == "nbAgg":
            if figure is not None:
                to_screen = output
                if widgets is not None:
                    to_screen = ipw.VBox([output, widgets])
                disp.display(to_screen)
                with output:
                    disp.display(figure)
            else:
                disp.display(widgets)
        elif figure is not None:
            disp.display(figure)
    return
