# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw


class PlotPanel:
    def __init__(self):

        self.container = ipw.VBox()
        self.interface = {}

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return self.container

    def connect(self, callbacks):
        for key, func in callbacks.items():
            self.interface[key] = func

    def rescale_to_data(self, vmin=None, vmax=None, mask_info=None):
        """
        Default dummy rescale function.
        """
        return
