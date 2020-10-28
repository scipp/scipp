# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw


class PlotToolbar:
    """
    Custom toolbar with additional buttons.
    """
    def __init__(self, fig_toolbar=None, swap_axes_button=False):
        self.container = ipw.VBox()
        self.members = {}
        if fig_toolbar is not None:
            self.members["original"] = fig_toolbar
        else:
            self.add_button("menu", "navicon", "Menu")
            self.add_button("home", "home", "Reset original view")
        self.add_button("rescale_to_data", "arrows-v", "Rescale")
        if swap_axes_button:
            self.add_button("swap_axes", "exchange", "Swap axes")
        self._update_container()

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
        # self.container.children = tuple(self.members.values())

    def connect(self, callbacks):
        for key in callbacks:
            if key in self.members:
                self.members[key].on_click(callbacks[key])
        # if "swap" in self.members:
        #     self.members["swap"].on_click(callbacks["swap_axes"])

    def _update_container(self):
        self.container.children = tuple(self.members.values())
