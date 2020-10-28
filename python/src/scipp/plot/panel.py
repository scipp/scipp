# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw


class PlotPanel:
    """
    Base class for providing additional widgets on top of the base dimension
    sliders and mask display control.
    """
    def __init__(self):
        self.container = ipw.VBox()
        self.interface = {}

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Get the panel widgets in a single container box for display.
        """
        return self.container

    def connect(self, callbacks):
        """
        Connect the panel interface to the callbacks provided by the
        `PlotController`.
        """
        for key, func in callbacks.items():
            self.interface[key] = func

    def rescale_to_data(self, vmin=None, vmax=None, mask_info=None):
        """
        Default dummy rescale function.
        """
        return
