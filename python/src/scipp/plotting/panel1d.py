# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .panel import PlotPanel
from ..utils import make_random_color
from .view1d import _make_label
import ipywidgets as ipw


class PlotPanel1d(PlotPanel):
    """
    Additional widgets that allow the duplication of the currently
    displayed line, a functionality inspired by the Superplot in the Lamp
    software.
    """
    def __init__(self, data_names):
        super().__init__()

        self.keep_buttons = {}
        self.data_names = data_names
        self.slice_label = None
        self.counter = -1

    def make_keep_button(self):
        """
        Dynamically create a new "keep" button, when a line is saved, as the
        old "keep" button gets converted to a "remove" button.
        """
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
                              layout={'width': "40px"})
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
        self.update_widgets()

    def update_axes(self):
        """
        Clear all widgets when figure axes are changed.
        """
        self.keep_buttons.clear()
        self.make_keep_button()
        self.update_widgets()

    def update_data(self, array):
        """
        Save label (slice position and thickness) of current line so that it
        can be labeled accordingly when the "keep" button is pressed.
        """
        self.slice_label = _make_label(array)

    def update_widgets(self):
        """
        Update the container of all widgets when a line is kept or removed.
        """
        widget_list = []
        for key, val in self.keep_buttons.items():
            widget_list.append(ipw.HBox(list(val.values())))
        self.container.children = tuple(widget_list)

    def keep_remove_line(self, owner):
        """
        Forward the keep or remove event to the correct function.
        """
        if owner.description == "Keep":
            self.keep_line(owner)
        elif owner.description == "Remove":
            self.remove_line(owner)

    def keep_line(self, owner):
        """
        Send a "keep line" event to the `PlotController`.
        """
        name = self.keep_buttons[owner.id]["dropdown"].value
        self.controller.keep_line(
            name=name,
            color=self.keep_buttons[owner.id]["colorpicker"].value,
            line_id=owner.id)
        self.keep_buttons[owner.id]["dropdown"].disabled = True
        self.keep_buttons[owner.id]["label"].value = self.slice_label
        self.make_keep_button()
        owner.description = "Remove"

    def remove_line(self, owner):
        """
        Send a "remove line" event to the `PlotController`.
        """
        self.controller.remove_line(line_id=owner.id)
        del self.keep_buttons[owner.id]
        self.update_widgets()

    def update_line_color(self, change):
        """
        Send a "update line color" event to the `PlotController`.
        """
        self.controller.update_line_color(change["owner"].id, change["new"])
