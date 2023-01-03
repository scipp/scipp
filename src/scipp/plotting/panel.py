# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw

from .displayable import Displayable


class PlotPanel(Displayable):
    """
    Base class for providing additional widgets on top of the base dimension
    sliders and mask display control.
    """

    def __init__(self):
        self.container = ipw.VBox()
        self.controller = None

    def _to_widget(self):
        """
        Get the panel widgets in a single container box for display.
        """
        return self.container

    def rescale_to_data(self, vmin=None, vmax=None, mask_info=None):
        """
        Default dummy rescale function.
        """
        return
