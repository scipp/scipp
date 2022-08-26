# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw
from .tools import widgets_have_mimebundle


class PlotPanel:
    """
    Base class for providing additional widgets on top of the base dimension
    sliders and mask display control.
    """

    def __init__(self):
        self.container = ipw.VBox()
        self.controller = None

    if widgets_have_mimebundle():

        def _repr_mimebundle_(self, include=None, exclude=None):
            """
            Mimebundle display representation for jupyter notebooks.
            """
            return self._to_widget()._repr_mimebundle_(include=include, exclude=exclude)
    else:

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

    def rescale_to_data(self, vmin=None, vmax=None, mask_info=None):
        """
        Default dummy rescale function.
        """
        return
