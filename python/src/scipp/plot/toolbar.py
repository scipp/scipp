# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw


class PlotToolbar:
    """
    Custom toolbar with additional buttons and back/forward buttons removed.
    """
    def __init__(self, canvas=None, ndim=1):
        # Prepare containers
        self.container = ipw.VBox()
        self.members = {}

        # Construct  toolbar
        if canvas is not None:
            old_tools = canvas.toolbar.toolitems
            new_tools = [old_tools[0], old_tools[3], old_tools[4], old_tools[5]]
            canvas.toolbar.toolitems = new_tools
            self.members["mpl_toolbar"] = canvas.toolbar
            canvas.toolbar_visible = False
        else:
            self.add_button(name="menu", icon="bars", tooltip="Menu")
            self.add_button(name="home", icon="home", tooltip="Reset original view")
        self.add_button(name="rescale_to_data", icon="arrows-v", tooltip="Rescale")
        if ndim > 1:
            self.add_button(name="swap_axes", icon="exchange", tooltip="Swap axes")

        self.add_togglebutton(name="toggle_xaxis_scale", description="logx", tooltip="Log(x)")
        # In the case of a 1d plot, we connect the logy button to the
        # toggle_norm function.
        if ndim == 1:
            self.add_togglebutton(name="toggle_norm", description="logy", tooltip="Log(y)")
        else:
            self.add_togglebutton(name="toggle_yaxis_scale", description="logy", tooltip="Log(y)")
            self.add_togglebutton(name="toggle_norm", icon="signal", tooltip="Log(data)")

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

    def add_button(self, name, **kwargs):
        """
        """
        args = self._parse_button_args(**kwargs)
        self.members[name] = ipw.Button(**args)

    def add_togglebutton(self, name, **kwargs):
        """
        """
        args = self._parse_button_args(**kwargs)
        self.members[name] = ipw.ToggleButton(value=False, **args)

    def connect(self, callbacks):
        for key in callbacks:
            if key in self.members:
                if hasattr(self.members[key], "on_click"):
                    self.members[key].on_click(callbacks[key])
                else:
                    self.members[key].observe(callbacks[key], names="value")

    def _update_container(self):
        # for group, item in self.containers.items():
        self.container.children = tuple(self.members.values())

    def _parse_button_args(self, layout=None, **kwargs):
        # layout = {"width": "34px"}
        # if "layout" in kwargs:
        #     layout.update(kwargs["layout"])
        args = {"layout": {"width": "34px", "padding": "0px 0px 0px 0px"}}
        if layout is not None:
            args["layout"].update(layout)
        for key, value in kwargs.items():
            if value is not None:
                args[key] = value
        return args
