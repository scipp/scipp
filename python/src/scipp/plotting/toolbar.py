# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw

from .. import config


def set_button_color(button, selected=False):
    name = 'button_selected' if selected else 'button'
    try:
        button.style.button_color = config['colors'][name]
    except KeyError:
        pass  # we do not have a color we can use


def _make_toggle_button(**kwargs):
    return ipw.ToggleButton(layout={
        "width": "34px",
        "padding": "0px 0px 0px 0px"
    },
                            **kwargs)


class PlotToolbar:
    """
    Custom toolbar with additional buttons for controlling log scales and
    normalization, and with back/forward buttons removed.
    """
    def __init__(self, mpl_toolbar):
        self._dims = None
        self.controller = None

        self.container = ipw.VBox()
        self.members = {}

        # Keep a reference to the matplotlib toolbar so we can call the zoom
        # and pan methods
        self.mpl_toolbar = mpl_toolbar

        self.add_button(name="home_view",
                        icon="home",
                        tooltip="Reset original view")

    def initialize(self, log_axis_buttons):
        self._log_axis = {
            dim: _make_toggle_button(tooltip=f'log({dim})')
            for dim in log_axis_buttons
        }

    @property
    def dims(self):
        return self._dims

    @dims.setter
    def dims(self, dims):
        if self._dims == dims:
            return
        self._dims = dims
        for dim, button in self._log_axis.items():
            if dim in self.dims:
                button.layout.display = ''
            else:
                button.layout.display = 'none'
        for ax, dim in zip('xy', dims[::-1]):  # might have only 1 dim -> x
            self._log_axis[dim].description = f'log{ax}'
            self.members[f'toggle_{ax}axis_scale'] = self._log_axis[dim]
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
        button = ipw.Button(**self._parse_button_args(**kwargs))
        set_button_color(button)
        self.members[name] = button

    def add_togglebutton(self, name, **kwargs):
        """
        Create a fake ToggleButton using Button because sometimes we want to
        change the value of the button without triggering an update, e.g. when
        we swap the axes.
        """
        button = ipw.Button(**self._parse_button_args(**kwargs))
        set_button_color(button)
        setattr(button, "value", False)
        # Add a local observer to change the color of the button according to
        # its value.
        button.on_click(self.toggle_button_color)
        self.members[name] = button

    def toggle_button_color(self, owner, value=None):
        """
        Change the color of the button to make it look like a ToggleButton.
        """
        if value is None:
            owner.value = not owner.value
        else:
            owner.value = value
        set_button_color(owner, selected=owner.value)

    def connect(self, controller):
        """
        Connect callbacks to button clicks.
        """
        for key in self.members:
            if hasattr(controller, key):
                self.members[key].on_click(getattr(controller, key))
            elif self.members[key] is not None:
                self.members[key].on_click(getattr(self, key))
        for dim, button in self._log_axis.items():
            button.observe(
                getattr(controller, 'toggle_dim_scale')(dim), 'value')

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

    def update_norm_button(self, norm=None):
        """
        Change state of norm button according to supplied norm value.
        """
        self.toggle_button_color(self.members["toggle_norm"],
                                 value=norm == "log")

    @property
    def tool_active(self):
        return self.members["zoom_view"].value or \
                self.members["pan_view"].value

    def home_view(self, button):
        self.mpl_toolbar.home()

    def pan_view(self, button):
        # In case the zoom button is selected, we need to de-select it
        if self.members["zoom_view"].value:
            self.toggle_button_color(self.members["zoom_view"])
        self.mpl_toolbar.pan()

    def zoom_view(self, button):
        # In case the pan button is selected, we need to de-select it
        if self.members["pan_view"].value:
            self.toggle_button_color(self.members["pan_view"])
        self.mpl_toolbar.zoom()

    def save_view(self, button):
        self.mpl_toolbar.save_figure()

    def rescale_on_zoom(self):
        return self.members["zoom_view"].value


class PlotToolbar1d(PlotToolbar):
    """
    Custom toolbar for 1d figures.
    """
    def __init__(self, *args, **kwargs):

        super().__init__(*args, **kwargs)

        self.add_togglebutton(name="pan_view", icon="arrows", tooltip="Pan")
        self.add_togglebutton(name="zoom_view",
                              icon="square-o",
                              tooltip="Zoom")
        self.add_button(name="rescale_to_data",
                        icon="arrows-v",
                        tooltip="Rescale")
        self.members['toggle_xaxis_scale'] = None
        self.add_togglebutton(name="toggle_norm",
                              description="logy",
                              tooltip="Log(y)")
        self.add_button(name="save_view", icon="save", tooltip="Save")


class PlotToolbar2d(PlotToolbar):
    """
    Custom toolbar for 2d figures.
    """
    def __init__(self, *args, **kwargs):

        super().__init__(*args, **kwargs)

        self.add_togglebutton(name="pan_view", icon="arrows", tooltip="Pan")
        self.add_togglebutton(name="zoom_view",
                              icon="square-o",
                              tooltip="Zoom")
        self.add_button(name="rescale_to_data",
                        icon="arrows-v",
                        tooltip="Rescale")
        self.add_button(name="transpose", icon="retweet", tooltip="Transpose")
        self.members['toggle_xaxis_scale'] = None
        self.members['toggle_yaxis_scale'] = None
        self.add_togglebutton(name="toggle_norm",
                              description="log",
                              tooltip="Log(data)")
        self.add_button(name="save_view", icon="save", tooltip="Save")


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
        self.add_button(name="rescale_to_data",
                        icon="arrows-v",
                        tooltip="Rescale")
        self.add_togglebutton(name="toggle_norm",
                              description="log",
                              tooltip="Log(data)")
