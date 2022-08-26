# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from functools import partial
from html import escape
import ipywidgets as ipw
from typing import Callable
from ..displayable import Displayable


class Checkboxes(Displayable):
    """
    Widget providing a list of checkboxes, along with a button to toggle them all.
    """

    def __init__(self, entries: list, description=""):

        self._callback = None

        self.checkboxes = {}
        self._lock = False

        for key in entries:
            self.checkboxes[key] = ipw.Checkbox(value=True,
                                                description=f"{escape(key)}",
                                                indent=False,
                                                layout={"width": "initial"})

        if len(self.checkboxes):
            self._description = ipw.Label(value=description)

            # Add a master button to control all masks in one go
            self.toggle_all_button = ipw.ToggleButton(value=True,
                                                      description="De-select all",
                                                      disabled=False,
                                                      button_style="",
                                                      layout={"width": "100px"})

    def to_widget(self) -> ipw.Widget:
        """
        Gather all widgets in a single container box.
        """
        out = ipw.HBox()
        if len(self.checkboxes):
            out.children = [
                self._description, self.toggle_all_button,
                ipw.HBox(list(self.checkboxes.values()))
            ]
        return out

    def observe(self, callback: Callable, **kwargs):
        for chbx in self.checkboxes.values():
            chbx.observe(self._toggle, **kwargs)
        if len(self.checkboxes):
            self.toggle_all_button.observe(self._toggle_all, **kwargs)
        self._callback = partial(callback, None)

    @property
    def value(self) -> dict:
        """
        """
        return {key: chbx.value for key, chbx in self.checkboxes.items()}

    def _toggle(self, _):
        if self._lock:
            return
        self._callback()

    def _toggle_all(self, change: dict):
        """
        A main button to hide or show all masks at once.
        """
        self._lock = True
        for key in self.checkboxes:
            self.checkboxes[key].value = change["new"]
        change["owner"].description = ("De-select all"
                                       if change["new"] else "Select all")
        self._lock = False
        self._callback()
