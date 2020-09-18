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
                 grid=False):
                 # ndim=None,
                 # data_names=None):

        self.controller = controller
        # self.slice_label = None

        self.profile_hover_connection = None
        self.profile_pick_connection = None
        self.profile_update_lock = False
        self.profile_scatter = None
        self.profile_counter = -1
        self.profile_ids = []



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

        # self.widgets = ipw.VBox()
        # self.keep_buttons = {}
        # self.data_names = data_names
        # self.make_keep_button()
        # if ndim < 2:
        #     self.additional_widgets.layout.display = 'none'

        return

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

    def _to_widget(self):
        # widgets_ = [self.figure, self.widgets]
        # if self.overview["widgets"] is not None:
        #     wdgts.append(self.overview["widgets"])
        # return ipw.VBox([self.figure._to_widget(), self.widgets])
        return self.figure._to_widget()


    def savefig(self, filename=None):
        self.figure.savefig(filename=filename)


    # def make_keep_button(self):
    #     drop = ipw.Dropdown(options=self.data_names,
    #                             description='',
    #                             layout={'width': 'initial'})
    #     lab = ipw.Label()
    #     but = ipw.Button(description="Keep",
    #                          disabled=False,
    #                          button_style="",
    #                          layout={'width': "70px"})
    #     # Generate a random color. TODO: should we initialise the seed?
    #     col = ipw.ColorPicker(concise=True,
    #                               description='',
    #                               value='#%02X%02X%02X' %
    #                               (tuple(np.random.randint(0, 255, 3))),
    #                               disabled=False)
    #     # Make a unique id
    #     key = str(id(but))
    #     setattr(but, "id", key)
    #     setattr(col, "id", key)
    #     but.on_click(self.keep_remove_trace)
    #     col.observe(self.update_trace_color, names="value")
    #     self.keep_buttons[key] = {
    #         "dropdown": drop,
    #         "button": but,
    #         "colorpicker": col,
    #         "label": lab
    #     }
    #     self.widgets.children += ipw.HBox(list(self.keep_buttons[key].values())),
    #     return

    # def clear_keep_buttons(self):
    #     self.keep_buttons.clear()
    #     self.make_keep_button()
    #     self.update_widgets()

    # # def update_buttons(self, owner, event, dummy):
    # #     for dim, button in self.buttons.items():
    # #         if dim == owner.dim:
    # #             self.slider[dim].disabled = True
    # #             button.disabled = True
    # #             self.button_axis_to_dim["x"] = dim
    # #         else:
    # #             self.slider[dim].disabled = False
    # #             button.value = None
    # #             button.disabled = False
    # #     self.update_axes(owner.dim)
    # #     self.keep_buttons = dict()
    # #     self.make_keep_button()
    # #     self.update_button_box_widget()
    # #     return

    # def update_widgets(self):
    #     # for k, b in self.keep_buttons.items():
    #     #     self.mbox.append(widgets.HBox(list(b.values())))
    #     # self.box.children = tuple(self.mbox)
    #     widget_list = []
    #     for key, val in self.keep_buttons.items():
    #         widget_list.append(ipw.HBox(list(val.values())))
    #     self.widgets.children = tuple(widget_list)



    # def keep_remove_trace(self, owner):
    #     if owner.description == "Keep":
    #         self.keep_trace(owner)
    #     elif owner.description == "Remove":
    #         self.remove_trace(owner)
    #     # self.fig.canvas.draw_idle()
    #     return

    # def keep_trace(self, owner):
    #     name = self.keep_buttons[owner.id]["dropdown"].value

    #     self.figure.keep_line(name=name, color=self.keep_buttons[owner.id]["colorpicker"].value,
    #         line_id=owner.id)

    #     # for dim, val in self.widgets.slider.items():
    #     #     if not val.disabled:
    #     #         lab = "{},{}:{}".format(lab, dim, val.value)
    #     # self.keep_buttons[owner.id]["dropdown"].options = name + self.controller.slice_label
    #     # self.keep_buttons[owner.id]["dropdown"].layout.width = 'initial'
    #     self.keep_buttons[owner.id]["dropdown"].disabled = True
    #     self.keep_buttons[owner.id]["label"].value = self.controller.slice_label[1:]
    #     self.make_keep_button()
    #     owner.description = "Remove"
    #     # self.update_button_box_widget()
    #     return

    # def remove_trace(self, owner):
    #     self.figure.remove_line(owner.id)
    #     del self.keep_buttons[owner.id]
    #     self.update_widgets()
    #     return

    # def update_trace_color(self, change):
    #     self.figure.update_line_color(change["owner"].id, change["new"])
    #     return


    def toggle_mask(self, change):
        self.figure.toggle_mask(change["owner"].mask_group,
            change["owner"].mask_name, change["new"])
        return

    def rescale_to_data(self, vmin=None, vmax=None):
        self.figure.rescale_to_data()
        return





    def update_axes(self, axparams, axformatter, axlocator, logx, logy):

        self.figure.update_axes(axparams, axformatter, axlocator, logx, logy)
        # self.clear_keep_buttons()


    def update_data(self, new_values):
        self.figure.update_data(new_values)


    def keep_line(self, name=None, color=None, line_id=None):
        self.figure.keep_line(name=name, color=color, line_id=line_id)

    def remove_line(self, line_id):
        self.figure.remove_line(line_id)

    def update_line_color(self, line_id, color):
        self.figure.update_line_color(line_id, color)















    def reset_profile(self):
        if self.profile_scatter is not None:
            self.profile_scatter = None
            self.ax.collections = []
            self.fig.canvas.draw_idle()

    def update_profile(self, event):
        # os.write(1, "view1d: update_profile 1\n".encode())
        # ev = event.mouseevent

        if event.inaxes == self.ax:
            # os.write(1, "view1d: update_profile 2\n".encode())
            event.xdata = event.xdata - self.current_lims["x"][0]
            event.ydata = event.ydata - self.current_lims["y"][0]
            # os.write(1, "view1d: update_profile 3\n".encode())
            self.controller.update_profile(event)
            # os.write(1, "view1d: update_profile 4\n".encode())
            self.controller.toggle_hover_visibility(True)
        else:
            self.controller.toggle_hover_visibility(False)


    def keep_or_remove_profile(self, event):
        os.write(1, "view1d: keep_or_delete_profile 1\n".encode())
        if isinstance(event.artist, PathCollection):
            os.write(1, "view1d: keep_or_delete_profile 2\n".encode())
            self.remove_profile(event)
            os.write(1, "view1d: keep_or_delete_profile 3\n".encode())
            # We need a profile lock to catch the second time the function is
            # called because the pick event is registed by both the scatter
            # points and the image
            self.profile_update_lock = True
            os.write(1, "view1d: keep_or_delete_profile 4\n".encode())
        elif self.profile_update_lock:
            os.write(1, "view1d: keep_or_delete_profile 5\n".encode())
            self.profile_update_lock = False
            os.write(1, "view1d: keep_or_delete_profile 6\n".encode())
        else:
            os.write(1, "view1d: keep_or_delete_profile 7\n".encode())
            self.keep_profile(event)
            os.write(1, "view1d: keep_or_delete_profile 8\n".encode())
        self.fig.canvas.draw_idle()


    def update_profile_connection(self, visible):
        # Connect picking events
        if visible:
            self.profile_pick_connection = self.fig.canvas.mpl_connect('pick_event', self.keep_or_remove_profile)
            self.profile_hover_connection = self.fig.canvas.mpl_connect('motion_notify_event', self.update_profile)
            # self.profile_hover_connection = self.fig.canvas.mpl_connect('pick_event', self.update_profile)
        else:
            if self.profile_pick_connection is not None:
                self.fig.canvas.mpl_disconnect(self.profile_pick_connection)
            if self.profile_hover_connection is not None:
                self.fig.canvas.mpl_disconnect(self.profile_hover_connection)


    def keep_profile(self, event):
        # trace = list(
        #     self.profile_viewer[self.profile_key].keep_buttons.values())[-1]
        os.write(1, "view1d: keep_profile 1\n".encode())
        xdata = event.mouseevent.xdata
        ydata = event.mouseevent.ydata
        col = make_random_color(fmt='rgba')
        os.write(1, "view1d: keep_profile 2\n".encode())
        self.profile_counter += 1
        line_id = self.profile_counter
        self.profile_ids.append(line_id)
        os.write(1, "view1d: keep_profile 3\n".encode())
        if self.profile_scatter is None:
            self.profile_scatter = self.ax.scatter(
                [xdata], [ydata], c=[col], picker=5)
        else:
            new_offsets = np.concatenate(
                (self.profile_scatter.get_offsets(), [[xdata, ydata]]), axis=0)
            # col = np.array(_hex_to_rgb(trace["colorpicker"].value) + [255],
            #                dtype=np.float) / 255.0
            new_colors = np.concatenate(
                (self.profile_scatter.get_facecolors(), [col]), axis=0)
            self.profile_scatter.set_offsets(new_offsets)
            self.profile_scatter.set_facecolors(new_colors)
        os.write(1, "view1d: keep_profile 4\n".encode())
        # self.fig.canvas.draw_idle()

        self.controller.keep_line(view="profile", color=col, line_id=line_id)
        os.write(1, "view1d: keep_profile 5\n".encode())



    def remove_profile(self, event):
        os.write(1, "view1d: remove_profile 1\n".encode())
        ind = event.ind[0]
        os.write(1, "view1d: remove_profile 2\n".encode())
        xy = np.delete(self.profile_scatter.get_offsets(), ind, axis=0)
        os.write(1, "view1d: remove_profile 3\n".encode())
        c = np.delete(self.profile_scatter.get_facecolors(), ind, axis=0)
        os.write(1, "view1d: remove_profile 4\n".encode())
        self.profile_scatter.set_offsets(xy)
        os.write(1, "view1d: remove_profile 5\n".encode())
        self.profile_scatter.set_facecolors(c)
        # self.fig.canvas.draw_idle()
        os.write(1, "view1d: remove_profile 6\n".encode())

        # Also remove the line from the 1d plot
        self.controller.remove_line(view="profile", line_id=self.profile_ids[ind])
        os.write(1, "view1d: remove_profile 7\n".encode())
        self.profile_ids.pop(ind)
        os.write(1, "view1d: remove_profile 8\n".encode())