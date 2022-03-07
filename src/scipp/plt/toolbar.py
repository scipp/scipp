# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import ipywidgets as ipw
from .. import config


class PlotToolbar:
    """
    Custom toolbar with additional buttons for controlling log scales and
    normalization, and with back/forward buttons removed.
    """
    def __init__(self, external_toolbar=None):
        self._dims = None
        self.controller = None

        self.container = ipw.VBox()
        self.members = {}

        # Keep a reference to the matplotlib toolbar so we can call the zoom
        # and pan methods
        self.external_toolbar = external_toolbar

        self.add_button(name="home_view", icon="home", tooltip="Reset original view")

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
        button = ipw.Button(**self._parse_button_args(**kwargs))
        self.members[name] = button

    def add_togglebutton(self, name, value=False, **kwargs):
        """
        Create a fake ToggleButton using Button because sometimes we want to
        change the value of the button without triggering an update, e.g. when
        we swap the axes.
        """
        button = ipw.ToggleButton(layout={
            "width": "34px",
            "padding": "0px 0px 0px 0px"
        },
                                  value=value,
                                  **kwargs)
        self.members[name] = button

    def connect(self, view):
        """
        Connect callbacks to button clicks.
        """
        for key, button in self.members.items():
            obj = self if hasattr(self, key) else view
            callback = getattr(obj, key)
            if isinstance(button, ipw.ToggleButton):
                button.observe(callback, names='value')
            else:
                button.on_click(callback)

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

    @property
    def tool_active(self):
        return self.members["zoom_view"].value or \
                self.members["pan_view"].value

    def home_view(self, button):
        self.external_toolbar.home()

    def pan_view(self, change):
        if change["new"]:
            # In case the zoom button is selected, we need to de-select it
            if self.members["zoom_view"].value:
                self.members["zoom_view"].value = False
            self.external_toolbar.pan()

    def zoom_view(self, change):
        if change["new"]:
            # In case the pan button is selected, we need to de-select it
            if self.members["pan_view"].value:
                self.members["pan_view"].value = False
            self.external_toolbar.zoom()

    def save_view(self, button):
        self.external_toolbar.save_figure()

    def rescale_on_zoom(self):
        return self.members["zoom_view"].value


class PlotToolbar1d(PlotToolbar):
    """
    Custom toolbar for 1d figures.
    """
    def __init__(self, *args, **kwargs):

        super().__init__(*args, **kwargs)

        self.add_togglebutton(name="pan_view", icon="arrows", tooltip="Pan")
        self.add_togglebutton(name="zoom_view", icon="square-o", tooltip="Zoom")
        self.add_button(name="rescale_to_data", icon="arrows-v", tooltip="Rescale")
        self.add_togglebutton('toggle_xaxis_scale', description="logx")
        self.add_togglebutton(name="toggle_norm",
                              description="logy",
                              tooltip="log(data)")
        self.add_button(name="save_view", icon="save", tooltip="Save")
        self._update_container()


class PlotToolbar2d(PlotToolbar):
    """
    Custom toolbar for 2d figures.
    """
    def __init__(self, *args, **kwargs):

        super().__init__(*args, **kwargs)

        self.add_togglebutton(name="pan_view", icon="arrows", tooltip="Pan")
        self.add_togglebutton(name="zoom_view", icon="square-o", tooltip="Zoom")
        self.add_button(name="rescale_to_data", icon="arrows-v", tooltip="Rescale")
        self.add_button(name="transpose", icon="retweet", tooltip="Transpose")
        self.add_togglebutton('toggle_xaxis_scale', description="logx")
        self.add_togglebutton('toggle_yaxis_scale', description="logy")
        self.add_togglebutton(name="toggle_norm",
                              description="log",
                              tooltip="log(data)")
        self.add_button(name="save_view", icon="save", tooltip="Save")
        self._update_container()


class PlotToolbar3d(PlotToolbar):
    """
    Custom toolbar for 3d figures.
    """
    def __init__(self, *args, **kwargs):

        super().__init__(*args, **kwargs)
        self.add_button(name="camera_x_normal",
                        icon="camera",
                        description="X",
                        tooltip="Camera to X normal. "
                        "Click twice to flip the view direction.")
        self.add_button(name="camera_y_normal",
                        icon="camera",
                        description="Y",
                        tooltip="Camera to Y normal. "
                        "Click twice to flip the view direction.")
        self.add_button(name="camera_z_normal",
                        icon="camera",
                        description="Z",
                        tooltip="Camera to Z normal. "
                        "Click twice to flip the view direction.")
        self.add_togglebutton(name="toggle_axes_helper",
                              value=True,
                              description="\u27C0",
                              style={"font_weight": "bold"},
                              tooltip="Toggle visibility of XYZ axes")
        self.add_togglebutton(name="toggle_outline",
                              value=True,
                              icon="codepen",
                              tooltip="Toggle visibility of outline box")
        self.add_button(name="rescale_to_data", icon="arrows-v", tooltip="Rescale")
        self.add_togglebutton(name="toggle_norm",
                              description="log",
                              tooltip="log(data)")

    def home_view(self, button):
        self.external_toolbar.reset_camera()

    def camera_x_normal(self, button):
        self.external_toolbar.camera_x_normal()

    def camera_y_normal(self, button):
        self.external_toolbar.camera_y_normal()

    def camera_z_normal(self, button):
        self.external_toolbar.camera_z_normal()

    def toggle_axes_helper(self, button):
        self.external_toolbar.toggle_axes_helper(button.value)

    def toggle_outline(self, button):
        self.external_toolbar.toggle_outline(button.value)
