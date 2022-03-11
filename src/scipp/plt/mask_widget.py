# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from functools import partial
from html import escape
from ..utils import value_to_string


class MaskWidget:
    """
    Widget providing buttons to hide/show masks.
    """
    def __init__(self, masks):

        import ipywidgets as ipw

        self._checkboxes = {}
        for key, mask in masks.items():
            self._checkboxes[key] = ipw.Checkbox(value=True,
                                                 description=f"{escape(key)}",
                                                 indent=False,
                                                 layout={"width": "initial"})
            setattr(self._checkboxes[key], "mask_name", key)

        if len(self._checkboxes) > 0:
            self._label = ipw.Label(value="Masks:")

            # Add a master button to control all masks in one go
            self._all_masks_button = ipw.ToggleButton(value=True,
                                                      description="Hide all",
                                                      disabled=False,
                                                      button_style="",
                                                      layout={"width": "initial"})
            self._all_masks_button.observe(self._toggle_all_masks, keys="value")

            self._box_layout = ipw.Layout(display='flex',
                                          flex_flow='row wrap',
                                          align_items='stretch',
                                          width='70%')
            # mask_list = []
            # for key in self.mask_checkboxes:
            #     for cbox in self.mask_checkboxes[key].values():
            #         mask_list.append(cbox)

            # masks_box = ipw.Box(children=mask_list, layout=box_layout)
            # self._container += [
            #     ipw.HBox([self.masks_lab, self.all_masks_button, self.masks_box])
            # ]

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
        if len(self._checkboxes) > 0:
            out.children = [
                self._label, self._all_masks_button,
                ipw.Box(children=self._checkboxes.values(), layout=self.box_layout)
            ]
        return out

    def _toggle_all_masks(self, change):
        """
        A main button to hide or show all masks at once.
        """
        for key in self._checkboxes:
            self._checkboxes[key].value = change["new"]
        change["owner"].description = "Hide all" if change["new"] else \
            "Show all"

    def connect(self, controller):
        """
        Connect the widget interface to the callbacks provided by the
        `PlotController`.
        """
        for key in self._checkboxes:
            self._checkboxes[key].observe(controller.toggle_mask, keys="value")

    def values(self):
        """
        Get information on masks: their keys and whether they should be
        displayed.
        """
        # mask_info = {}
        # for key in self.mask_checkboxes:
        #     mask_info[key] = {
        #         m: chbx.value
        #         for m, chbx in self.mask_checkboxes[key].items()
        #     }
        return {key: chbx.value for key, chbx in self._checkboxes.items()}
