
from .._utils import make_random_color

import ipywidgets as ipw
import numpy as np


class PlotPanel1d:

    def __init__(self, controller, data_names):

        self.controller = controller
        self.widgets = ipw.VBox()
        self.keep_buttons = {}
        self.data_names = data_names
        self.slice_label= None
        self.counter = -1
        # self.make_keep_button()
        # if ndim < 2:
        #     self.widgets.layout.display = 'none'

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        return self.widgets

    def make_keep_button(self):
        drop = ipw.Dropdown(options=self.data_names,
                                description='',
                                layout={'width': 'initial'})
        lab = ipw.Label()
        but = ipw.Button(description="Keep",
                             disabled=False,
                             button_style="",
                             layout={'width': "70px"})
        # Generate a random color. TODO: should we initialise the seed?
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
        self.widgets.children += ipw.HBox(list(self.keep_buttons[line_id].values())),
        return

    def update_axes(self, axparams=None):
        self.keep_buttons.clear()
        self.make_keep_button()
        self.update_widgets()

    def update_data(self, info):
        self.slice_label = info["slice_label"][1:]

    # def update_buttons(self, owner, event, dummy):
    #     for dim, button in self.buttons.items():
    #         if dim == owner.dim:
    #             self.slider[dim].disabled = True
    #             button.disabled = True
    #             self.button_axis_to_dim["x"] = dim
    #         else:
    #             self.slider[dim].disabled = False
    #             button.value = None
    #             button.disabled = False
    #     self.update_axes(owner.dim)
    #     self.keep_buttons = dict()
    #     self.make_keep_button()
    #     self.update_button_box_widget()
    #     return

    def update_widgets(self):
        # for k, b in self.keep_buttons.items():
        #     self.mbox.append(widgets.HBox(list(b.values())))
        # self.box.children = tuple(self.mbox)
        widget_list = []
        for key, val in self.keep_buttons.items():
            widget_list.append(ipw.HBox(list(val.values())))
        self.widgets.children = tuple(widget_list)



    def keep_remove_line(self, owner):
        if owner.description == "Keep":
            self.keep_line(owner)
        elif owner.description == "Remove":
            self.remove_line(owner)
        # self.fig.canvas.draw_idle()
        return

    def keep_line(self, owner):
        name = self.keep_buttons[owner.id]["dropdown"].value

        # self.figure.keep_line(name=name, color=self.keep_buttons[owner.id]["colorpicker"].value,
        #     line_id=owner.id)
        self.controller.keep_line(name=name, color=self.keep_buttons[owner.id]["colorpicker"].value,
            line_id=owner.id)

        # for dim, val in self.widgets.slider.items():
        #     if not val.disabled:
        #         lab = "{},{}:{}".format(lab, dim, val.value)
        # self.keep_buttons[owner.id]["dropdown"].options = name + self.controller.slice_label
        # self.keep_buttons[owner.id]["dropdown"].layout.width = 'initial'
        self.keep_buttons[owner.id]["dropdown"].disabled = True
        self.keep_buttons[owner.id]["label"].value = self.slice_label
        self.make_keep_button()
        owner.description = "Remove"
        # self.update_button_box_widget()
        return

    def remove_line(self, owner):
        self.controller.remove_line(line_id=owner.id)
        del self.keep_buttons[owner.id]
        self.update_widgets()
        return

    def update_line_color(self, change):
        self.controller.update_line_color(change["owner"].id, change["new"])
        return


    def rescale_to_data(self, vmin=None, vmax=None, mask_info=None):
        return

    def toggle_mask(self, mask_info=None):
        return
