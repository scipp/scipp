# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from functools import partial
from html import escape
from ..utils import value_to_string
from .step import Step


class MaskWidget:
    """
    Widget providing buttons to hide/show masks.
    """
    def __init__(self, masks, name):

        import ipywidgets as ipw
        self._callback = None

        self._checkboxes = {}
        self._lock = False
        for key, mask in masks.items():
            self._checkboxes[key] = ipw.Checkbox(value=True,
                                                 description=f"{escape(key)}",
                                                 indent=False,
                                                 layout={"width": "initial"})
            self._checkboxes[key].observe(self._toggle_mask, names="value")

        if len(self._checkboxes):
            self._label = ipw.Label(value=f"Masks: {name}")

            # Add a master button to control all masks in one go
            self._all_masks_button = ipw.ToggleButton(value=True,
                                                      description="Hide all",
                                                      disabled=False,
                                                      button_style="",
                                                      layout={"width": "initial"})
            self._all_masks_button.observe(self._toggle_all_masks, names="value")

            self._box_layout = ipw.Layout(display='flex',
                                          flex_flow='row wrap',
                                          align_items='stretch',
                                          width='70%')

    def _ipython_display_(self):
        """
        IPython display representation for Jupyter notebooks.
        """
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        """
        Gather all widgets in a single container box.
        """
        import ipywidgets as ipw
        out = ipw.HBox()
        if len(self._checkboxes):
            out.children = [
                self._label, self._all_masks_button,
                ipw.Box(children=list(self._checkboxes.values()),
                        layout=self._box_layout)
            ]
        return out

    def set_callback(self, callback):
        self._callback = callback

    def values(self):
        """
        Get information on masks: their keys and whether they should be
        displayed.
        """
        return {key: chbx.value for key, chbx in self._checkboxes.items()}

    def _toggle_mask(self, _):
        if self._lock:
            return
        self._callback()

    def _toggle_all_masks(self, change):
        """
        A main button to hide or show all masks at once.
        """
        self._lock = True
        for key in self._checkboxes:
            self._checkboxes[key].value = change["new"]
        change["owner"].description = "Hide all" if change["new"] else \
            "Show all"
        self._lock = False
        # self._toggle_mask(None)
        self._callback()


def _hide_masks(model, masks):
    out = model.copy()
    for name, value in masks.items():
        if not value:
            del out.masks[name]
    return out


class MaskStep(Step):
    def __init__(self, **kwargs):
        super().__init__(func=_hide_masks, widget=MaskWidget(**kwargs))
