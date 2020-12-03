# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw


class PlotToolbar:
    """
    Custom toolbar with additional buttons for controlling log scales and
    normalization, and with back/forward buttons removed.
    """
    def __init__(self, canvas=None, ndim=1):
        # Prepare containers
        self.container = ipw.VBox()
        self.members = {}
        self.mpl_toolbar = None


        # canvas.toolbar if canvas is not None else None

        # Construct  toolbar
        if canvas is not None:
            # old_tools = canvas.toolbar.toolitems
            # new_tools = [
            #     old_tools[0], old_tools[3], old_tools[4], old_tools[5]
            # ]
            # canvas.toolbar.toolitems = new_tools
            # self.members["mpl_toolbar"] = canvas.toolbar
            canvas.toolbar_visible = False
            self.mpl_toolbar = canvas.toolbar
        # else:
            # self.add_button(name="menu", icon="bars", tooltip="Menu")

        self.add_button(name="home_view",
                        icon="home",
                        tooltip="Reset original view")

        if ndim < 3:
            self.add_togglebutton(name="pan_view",
                            icon="arrows",
                            tooltip="Pan")

            self.add_togglebutton(name="zoom_view",
                            icon="square-o",
                            tooltip="Zoom")

        self.add_button(name="rescale_to_data",
                        icon="arrows-v",
                        tooltip="Rescale")
        if ndim == 2:
            self.add_button(name="transpose",
                            icon="retweet",
                            tooltip="Transpose")

        if ndim < 3:
            self.add_togglebutton(name="toggle_xaxis_scale",
                                  description="logx",
                                  tooltip="Log(x)")
        if ndim == 1:
            # In the case of a 1d plot, we connect the logy button to the
            # toggle_norm function.
            self.add_togglebutton(name="toggle_norm",
                                  description="logy",
                                  tooltip="Log(y)")
        else:
            if ndim < 3:
                self.add_togglebutton(name="toggle_yaxis_scale",
                                      description="logy",
                                      tooltip="Log(y)")
            self.add_togglebutton(name="toggle_norm",
                                  description="log",
                                  tooltip="Log(data)")

        if ndim < 3:
            self.add_button(name="save_view",
                            icon="save",
                            tooltip="Save")


        self._update_container()

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Return the VBox container
        """
        return self.container

    def set_visible(self, visible):
        """
        Need to hide/show the toolbar when a canvas is hidden/shown.
        """
        self.container.layout.display = None if visible else 'none'

    def add_button(self, name, **kwargs):
        """
        Create a new button and add it to the toolbar members list.
        """
        args = self._parse_button_args(**kwargs)
        self.members[name] = ipw.Button(**args)

    def add_togglebutton(self, name, **kwargs):
        """
        Create a fake ToggleButton using Button because sometimes we want to
        change the value of the button without triggering an update, e.g. when
        we swap the axes.
        """
        args = self._parse_button_args(**kwargs)
        self.members[name] = ipw.Button(**args)
        setattr(self.members[name], "value", False)
        # Add a local observer to change the color of the button according to
        # its value.
        self.members[name].on_click(self.toggle_button_color)

    def toggle_button_color(self, owner, value=None):
        """
        Change the color of the button to make it look like a ToggleButton.
        """
        if value is None:
            owner.value = not owner.value
        else:
            owner.value = value
        owner.style.button_color = "#bdbdbd" if owner.value else "#eeeeee"

    def connect(self, callbacks):
        """
        Connect callbacks to button clicks.
        """
        for key in callbacks:
            if key in self.members:
                self.members[key].on_click(callbacks[key])

    def _update_container(self):
        """
        Update the container's children according to the buttons in the
        members.
        """
        self.container.children = tuple(self.members.values())

    def _parse_button_args(self, layout=None, **kwargs):
        """
        Parse button arguments and add some default styling options.
        """
        args = {"layout": {"width": "34px", "padding": "0px 0px 0px 0px"}}
        if layout is not None:
            args["layout"].update(layout)
        for key, value in kwargs.items():
            if value is not None:
                args[key] = value
        return args

    def update_log_axes_buttons(self, axes_scales):
        """
        When axes are changed or swapped, update the value and color of the
        custom togglebuttons without triggering a new update.
        """
        for xyz, scale in axes_scales.items():
            key = "toggle_{}axis_scale".format(xyz)
            if key in self.members:
                self.toggle_button_color(self.members[key],
                                         value=scale == "log")

    def update_norm_button(self, norm=None):
        """
        Change state of norm button according to supplied norm value.
        """
        self.toggle_button_color(self.members["toggle_norm"],
                                 value=norm == "log")

    def home_view(self):
        self.mpl_toolbar.home()

    def pan_view(self):
        # In case the zoom button is selected, we need to de-select it
        if self.members["zoom_view"].value:
            self.toggle_button_color(self.members["zoom_view"])
        self.mpl_toolbar.pan()

    def zoom_view(self):
        # In case the pan button is selected, we need to de-select it
        if self.members["pan_view"].value:
            self.toggle_button_color(self.members["pan_view"])
        self.mpl_toolbar.zoom()

    def save_view(self):
        self.mpl_toolbar.save_figure()

    def rescale_on_zoom(self):
        if self.members["zoom_view"].value:
            # Simulate a click on the rescale_to_data button
            self.members["rescale_to_data"].click()
