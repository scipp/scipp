# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw


class PlotToolbar:
    """
    Custom toolbar with additional buttons.
    """
    def __init__(self, fig_toolbar):
        self.members = {"original": fig_toolbar}
        self.container = ipw.VBox(list(self.members.values()))
        self.add_button("rescale", "arrows-v", "Rescale")
        self.add_button("swap", "exchange", "Swap axes")

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        """
        return self.container

    def set_visible(self, visible):
        """
        """
        self.container.layout.display = None if visible else 'none'

    def add_button(self, name, icon, tooltip=None):
        """
        """
        self.members[name] = ipw.Button(icon=icon, layout={"width": "34px"}, tooltip=tooltip)
        self.container.children = tuple(self.members.values())

    def connect(self, callbacks):
        self.members["rescale"].on_click(callbacks["rescale_to_data"])
        self.members["swap"].on_click(callbacks["swap_axes"])
