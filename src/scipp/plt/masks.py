# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .. import DataArray
from .filters import WidgetFilter
from ..typing import MetaDataMap

from functools import partial
from html import escape
import ipywidgets as ipw
from typing import Callable


class MaskWidget:
    """
    Widget providing buttons to hide/show masks.
    """
    def __init__(self, masks: MetaDataMap, name: str = ''):

        self._callback = None

        self._checkboxes = {}
        self._lock = False
        for key, mask in masks.items():
            self._checkboxes[key] = ipw.Checkbox(value=True,
                                                 description=f"{escape(key)}",
                                                 indent=False,
                                                 layout={"width": "initial"})

        if len(self._checkboxes):
            self._label = ipw.Label(value=f"Masks: {name}")

            # Add a master button to control all masks in one go
            self._all_masks_button = ipw.ToggleButton(value=True,
                                                      description="Hide all",
                                                      disabled=False,
                                                      button_style="",
                                                      layout={"width": "initial"})

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
                self._label, self._all_masks_button,
                ipw.Box(children=list(self._checkboxes.values()),
                        layout=self._box_layout)
            ]
        return out

    def observe(self, callback: Callable, **kwargs):
        for chbx in self._checkboxes.values():
            chbx.observe(self._toggle_mask, **kwargs)
        if len(self._checkboxes):
            self._all_masks_button.observe(self._toggle_all_masks, **kwargs)
        self._callback = partial(callback, None)

    @property
    def value(self) -> dict:
        """
        """
        return {key: chbx.value for key, chbx in self._checkboxes.items()}

    def _toggle_mask(self, _):
        if self._lock:
            return
        self._callback()

    def _toggle_all_masks(self, change: dict):
        """
        A main button to hide or show all masks at once.
        """
        self._lock = True
        for key in self._checkboxes:
            self._checkboxes[key].value = change["new"]
        change["owner"].description = "Hide all" if change["new"] else \
            "Show all"
        self._lock = False
        self._callback()


def hide_masks(model: DataArray, masks: MetaDataMap) -> DataArray:
    out = model.copy(deep=False)
    for name, value in masks.items():
        if not value:
            del out.masks[name]
    return out


# class MaskFilter(WidgetFilter):
#     def __init__(self, **kwargs):
#         super().__init__(func=_hide_masks, widgets={"masks": MaskWidget(**kwargs)})
