# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import ipywidgets as ipw


class PlotToolbar:
    """
    Custom toolbar with additional buttons for controlling log scales and
    normalization, and with back/forward buttons removed.
    """
    def __init__(self, canvas=None):

        # Prepare containers
        self.container = ipw.VBox()
        self.members = {}

        # Keep a reference to the matplotlib toolbar so we can call the zoom
        # and pan methods
        self.mpl_toolbar = None

        if canvas is not None:
            canvas.toolbar_visible = False
            self.mpl_toolbar = canvas.toolbar

        self.add_button(name="home_view",
                        icon="home",
                        tooltip="Reset original view")

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
        self.members[name] = ipw.ToggleButton(**args)
        setattr(self.members[name], "value", False)

    def connect(self, callbacks):
        """
        Connect callbacks to button clicks.
        """
        def toggle_button_wrapper(callback):
            def callback_forward(state):
                callback(state['new'])

            return callback_forward

        for key in callbacks:
            if key in self.members:
                if isinstance(self.members[key], ipw.ToggleButton):
                    self.members[key].observe(
                        toggle_button_wrapper(callbacks[key]), 'value')
                else:
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

    def home_view(self):
        self.mpl_toolbar.home()

    def save_view(self):
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
        self.add_togglebutton(name="toggle_xaxis_scale",
                              description="logx",
                              tooltip="Log(x)")
        self.add_togglebutton(name="toggle_norm",
                              description="logy",
                              tooltip="Log(y)")
        self.add_button(name="save_view", icon="save", tooltip="Save")
        self._update_container()


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
        self.add_togglebutton(name="toggle_xaxis_scale",
                              description="logx",
                              tooltip="Log(x)")
        self.add_togglebutton(name="toggle_yaxis_scale",
                              description="logy",
                              tooltip="Log(y)")
        self.add_togglebutton(name="toggle_norm",
                              description="log",
                              tooltip="Log(data)")
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
        self.add_button(name="rescale_to_data",
                        icon="arrows-v",
                        tooltip="Rescale")
        self.add_togglebutton(name="toggle_norm",
                              description="log",
                              tooltip="Log(data)")
        self._update_container()
