# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from ... import DataArray
from ...typing import MetaDataMap
from ..model import node

from functools import partial
from html import escape
import ipywidgets as ipw
from typing import Callable


class Checkboxes:
    """
    Widget providing a list of checkboxes, along with a button to toggle them all.
    """

    def __init__(self, entries: list, description=""):

        self._callback = None

        self._checkboxes = {}
        self._lock = False

        for key in entries:
            self._checkboxes[key] = ipw.Checkbox(value=True,
                                                 description=f"{escape(key)}",
                                                 indent=False,
                                                 layout={"width": "initial"})

        if len(self._checkboxes):
            self._description = ipw.Label(value=description)

            # Add a master button to control all masks in one go
            self._toggle_all_button = ipw.ToggleButton(value=True,
                                                       description="De-select all",
                                                       disabled=False,
                                                       button_style="",
                                                       layout={"width": "100px"})

            self._box_layout = ipw.Layout(display='flex',
                                          flex_flow='row wrap',
                                          align_items='stretch',
                                          width='70%')

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self) -> ipw.Widget:
        """
        Gather all widgets in a single container box.
        """
        out = ipw.HBox()
        if len(self._checkboxes):
            out.children = [
                self._description, self._toggle_all_button,
                ipw.Box(children=list(self._checkboxes.values()),
                        layout=self._box_layout)
            ]
        return out

    def observe(self, callback: Callable, **kwargs):
        for chbx in self._checkboxes.values():
            chbx.observe(self._toggle, **kwargs)
        if len(self._checkboxes):
            self._toggle_all_button.observe(self._toggle_all, **kwargs)
        self._callback = partial(callback, None)

    @property
    def value(self) -> dict:
        """
        """
        return {key: chbx.value for key, chbx in self._checkboxes.items()}

    def _toggle(self, _):
        if self._lock:
            return
        self._callback()

    def _toggle_all(self, change: dict):
        """
        A main button to hide or show all masks at once.
        """
        self._lock = True
        for key in self._checkboxes:
            self._checkboxes[key].value = change["new"]
        change["owner"].description = ("De-select all"
                                       if change["new"] else "Select all")
        self._lock = False
        self._callback()


@node
def hide_masks(data_array: DataArray, masks: MetaDataMap) -> DataArray:
    out = data_array.copy(deep=False)
    for name, value in masks.items():
        if not value:
            del out.masks[name]
    return out
