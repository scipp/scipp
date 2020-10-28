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
            self.add_button(name="menu", icon="navicon", tooltip="Menu")
            self.add_button(name="home", icon="home", tooltip="Reset original view")
        self.add_button(name="rescale_to_data", icon="arrows-v", tooltip="Rescale")
        self.add_togglebutton(name="logx", description="Logx", tooltip="Log(x)")
        if swap_axes_button:
            self.add_button(name="swap_axes", icon="exchange", tooltip="Swap axes")
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

    def add_button(self, name, icon=None, description=None, tooltip=None):
        """
        """
        # args = {}
        # if icon is not None:
        #     args["icon"] = icon
        # if description is not None:
        #     args["description"] = description
        # if tooltip is not None:
        #     args["tooltip"] = tooltip
        args = self._parse_button_args(icon=icon, description=description, tooltip=tooltip)
        self.members[name] = ipw.Button(**args, layout={"width": "34px"})
        # self.container.children = tuple(self.members.values())

    def add_togglebutton(self, name, icon=None, description=None, tooltip=None):
        """
        """
        args = self._parse_button_args(icon=icon, description=description, tooltip=tooltip)
        self.members[name] = ipw.ToggleButton(value=False, layout={"width": "34px"}, **args)
        # self.container.children = tuple(self.members.values())

    def connect(self, callbacks):
        for key in callbacks:
            if key in self.members:
                self.members[key].on_click(callbacks[key])
        # if "swap" in self.members:
        #     self.members["swap"].on_click(callbacks["swap_axes"])

    def _update_container(self):
        self.container.children = tuple(self.members.values())

    def _parse_button_args(self, **kwargs):
        args = {}
        for key, value in kwargs.items():
            if value is not None:
                args[key] = value
        return args
