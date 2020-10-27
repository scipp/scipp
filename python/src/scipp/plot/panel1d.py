# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .panel import PlotPanel
from .._utils import make_random_color
import ipywidgets as ipw


class PlotPanel1d(PlotPanel):
    def __init__(self, data_names):
        super().__init__()

        self.keep_buttons = {}
        self.data_names = data_names
        self.slice_label = None
        self.counter = -1
        self.interface = {}

    def make_keep_button(self):
        drop = ipw.Dropdown(options=self.data_names,
                            description='',
                            layout={'width': 'initial'})
        lab = ipw.Label()
        but = ipw.Button(description="Keep",
                         disabled=False,
                         button_style="",
                         layout={'width': "70px"})
        # Generate a random color.
        # TODO: should we initialise the seed?
        col = ipw.ColorPicker(concise=True,
                              description='',
                              value=make_random_color(fmt='hex'),
                              disabled=False)
        # Make a unique id
        self.counter += 1
        line_id = self.counter
        setattr(but, "id", line_id)
        setattr(col, "id", line_id)
        but.on_click(self.keep_remove_line)
        col.observe(self.update_line_color, names="value")
        self.keep_buttons[line_id] = {
            "dropdown": drop,
            "button": but,
            "colorpicker": col,
            "label": lab
        }
        self.container.children += ipw.HBox(
            list(self.keep_buttons[line_id].values())),

    def update_axes(self, axparams=None):
        self.keep_buttons.clear()
        self.make_keep_button()
        self.update_widgets()

    def update_data(self, info):
        self.slice_label = info["slice_label"]

    def update_widgets(self):
        widget_list = []
        for key, val in self.keep_buttons.items():
            widget_list.append(ipw.HBox(list(val.values())))
        self.container.children = tuple(widget_list)

    def keep_remove_line(self, owner):
        if owner.description == "Keep":
            self.keep_line(owner)
        elif owner.description == "Remove":
            self.remove_line(owner)

    def keep_line(self, owner):
        name = self.keep_buttons[owner.id]["dropdown"].value
        self.interface["keep_line"](
            name=name,
            color=self.keep_buttons[owner.id]["colorpicker"].value,
            line_id=owner.id)
        self.keep_buttons[owner.id]["dropdown"].disabled = True
        self.keep_buttons[owner.id]["label"].value = self.slice_label
        self.make_keep_button()
        owner.description = "Remove"

    def remove_line(self, owner):
        self.interface["remove_line"](line_id=owner.id)
        del self.keep_buttons[owner.id]
        self.update_widgets()

    def update_line_color(self, change):
        self.interface["update_line_color"](change["owner"].id, change["new"])

    def rescale_to_data(self, vmin=None, vmax=None, mask_info=None):
        return

    # def toggle_mask(self, mask_info=None):
    #     return
