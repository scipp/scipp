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
        for name, mask in masks.items():
            self._checkboxes[name] = ipw.Checkbox(value=True,
                                                  description=f"{escape(name)}",
                                                  indent=False,
                                                  layout={"width": "initial"})
            setattr(self.mask_checkboxes[name][key], "mask_name", name)

        if len(self._checkboxes) > 0:
            self._label = ipw.Label(value="Masks:")

            # Add a master button to control all masks in one go
            self._all_masks_button = ipw.ToggleButton(value=True,
                                                      description="Hide all",
                                                      disabled=False,
                                                      button_style="",
                                                      layout={"width": "initial"})
            self.all_masks_button.observe(self._toggle_all_masks, names="value")

            self.box_layout = ipw.Layout(display='flex',
                                         flex_flow='row wrap',
                                         align_items='stretch',
                                         width='70%')
            # mask_list = []
            # for name in self.mask_checkboxes:
            #     for cbox in self.mask_checkboxes[name].values():
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
        return ipw.HBox([
            self.masks_label, self.all_masks_button,
            ipw.Box(children=self._checkboxes.values(), layout=self.box_layout)
        ])

    def _toggle_all_masks(self, change):
        """
        A main button to hide or show all masks at once.
        """
        for name in self.mask_checkboxes:
            for key in self.mask_checkboxes[name]:
                self.mask_checkboxes[name][key].value = change["new"]
        change["owner"].description = "Hide all" if change["new"] else \
            "Show all"
        return

    def connect(self, controller):
        """
        Connect the widget interface to the callbacks provided by the
        `PlotController`.
        """
        for mask in self._checkboxes:
            for m in self.mask_checkboxes[name]:
                self.mask_checkboxes[name][m].observe(controller.toggle_mask,
                                                      names="value")

    def get_masks_info(self):
        """
        Get information on masks: their names and whether they should be
        displayed.
        """
        mask_info = {}
        for name in self.mask_checkboxes:
            mask_info[name] = {
                m: chbx.value
                for m, chbx in self.mask_checkboxes[name].items()
            }
        return mask_info
