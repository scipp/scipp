# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
# from .profiler import Profiler
from .lineplot import LinePlot
from .tools import to_bin_edges, parse_params
# from .widgets import PlotWidgets
from .._utils import name_with_unit
from .._scipp import core as sc
from .. import detail

# Other imports
import numpy as np
import ipywidgets as ipw
import matplotlib.pyplot as plt
from matplotlib.axes import Subplot
import warnings
import io


class PlotView1d:
    def __init__(self,
                 # scipp_obj_dict=None,
                 # axes=None,
                 # masks=None,
                 controller=None,
                 ax=None,
                 errorbars=None,
                 title=None,
                 unit=None,
                 logx=False,
                 logy=False,
                 mask_params=None,
                 mask_names=None,
                 mpl_line_params=None,
                 grid=False,
                 ndim=None,
                 data_names=None):

        self.controller = controller
        # self.slice_label = None



        self.figure = LinePlot(errorbars=errorbars,
                 # masks=masks,
                 ax=ax,
                 mpl_line_params=mpl_line_params,
                 title=title,
                 unit=unit,
                 logx=logx,
                 logy=logy,
                 grid=grid,
                 mask_params=mask_params,
                 mask_names=mask_names)

        self.widgets = ipw.VBox()
        self.keep_buttons = {}
        self.data_names = data_names
        self.make_keep_button()
        if ndim < 2:
            self.additional_widgets.layout.display = 'none'

        return

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        # widgets_ = [self.figure, self.widgets]
        # if self.overview["widgets"] is not None:
        #     wdgts.append(self.overview["widgets"])
        return ipw.VBox([self.figure._to_widget(), self.widgets])
        # return self.figure._to_widget()


    def savefig(self, filename=None):
        self.figure.savefig(filename=filename)


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
                                  value='#%02X%02X%02X' %
                                  (tuple(np.random.randint(0, 255, 3))),
                                  disabled=False)
        # Make a unique id
        key = str(id(but))
        setattr(but, "id", key)
        setattr(col, "id", key)
        but.on_click(self.keep_remove_trace)
        col.observe(self.update_trace_color, names="value")
        self.keep_buttons[key] = {
            "dropdown": drop,
            "button": but,
            "colorpicker": col,
            "label": lab
        }
        self.widgets.children += ipw.HBox(list(self.keep_buttons[key].values())),
        return

    def clear_keep_buttons(self):
        self.keep_buttons.clear()
        self.make_keep_button()
        self.update_widgets()

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



    def keep_remove_trace(self, owner):
        if owner.description == "Keep":
            self.keep_trace(owner)
        elif owner.description == "Remove":
            self.remove_trace(owner)
        # self.fig.canvas.draw_idle()
        return

    def keep_trace(self, owner):
        name = self.keep_buttons[owner.id]["dropdown"].value

        self.figure.keep_line(name=name, color=self.keep_buttons[owner.id]["colorpicker"].value,
            line_id=owner.id)

        # for dim, val in self.widgets.slider.items():
        #     if not val.disabled:
        #         lab = "{},{}:{}".format(lab, dim, val.value)
        # self.keep_buttons[owner.id]["dropdown"].options = name + self.controller.slice_label
        # self.keep_buttons[owner.id]["dropdown"].layout.width = 'initial'
        self.keep_buttons[owner.id]["dropdown"].disabled = True
        self.keep_buttons[owner.id]["label"].value = self.controller.slice_label[1:]
        self.make_keep_button()
        owner.description = "Remove"
        # self.update_button_box_widget()
        return

    def remove_trace(self, owner):
        self.figure.remove_line(owner.id)
        del self.keep_buttons[owner.id]
        self.update_widgets()
        return

    def update_trace_color(self, change):
        self.figure.update_line_color(change["owner"].id, change["new"])
        return


    def toggle_mask(self, change):
        self.figure.toggle_mask(change["owner"].mask_group,
            change["owner"].mask_name, change["new"])
        return

    def rescale_to_data(self, vmin=None, vmax=None):
        self.figure.rescale_to_data()
        return





    def update_axes(self, axparams, axformatter, axlocator, logx, logy):

        self.figure.update_axes(axparams, axformatter, axlocator, logx, logy)
        self.clear_keep_buttons()


    def update_data(self, new_values):
        self.figure.update_data(new_values)


