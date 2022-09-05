# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

import ipywidgets as ipw
from typing import Callable
from .displayable import Displayable


class ToggleButtons(ipw.ToggleButtons):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.on_msg(self._reset)
        self._current_value = self.value

    def _reset(self, *ignored):
        """
        If the currently selected button is clicked again, set value to None.
        """
        if self.value == self._current_value:
            self.value = None
        self._current_value = self.value


class Toolbar(Displayable):
    """
    Custom toolbar with additional buttons for controlling log scales and
    normalization, and with back/forward buttons removed.
    """

    def __init__(self):
        self._dims = None
        self.controller = None
        self.members = {}

    def to_widget(self) -> ipw.Widget:
        """
        Return the VBox container
        """
        return ipw.VBox(tuple(self.members.values()))

    def add_button(self, name: str, callback: Callable, **kwargs):
        """
        Create a new button and add it to the toolbar members list.
        """
        button = ipw.Button(**self._parse_button_args(**kwargs))
        button.on_click(callback)
        self.members[name] = button

    def add_togglebutton(self,
                         name: str,
                         callback: Callable,
                         value: bool = False,
                         **kwargs):
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
        button.observe(callback, names='value')
        self.members[name] = button

    def add_togglebuttons(self, name: str, callback: Callable, value=None, **kwargs):
        """
        Create a fake ToggleButton using Button because sometimes we want to
        change the value of the button without triggering an update, e.g. when
        we swap the axes.
        """
        buttons = ToggleButtons(button_style='',
                                layout={
                                    "width": "34px",
                                    "padding": "0px 0px 0px 0px"
                                },
                                style={
                                    "button_width": "20px",
                                    "button_padding": "0px 0px 0px 0px"
                                },
                                value=value,
                                **kwargs)
        buttons.observe(callback, names='value')
        self.members[name] = buttons

    def _parse_button_args(self, layout: dict = None, **kwargs) -> dict:
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
