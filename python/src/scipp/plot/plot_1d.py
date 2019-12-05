# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .render import render_plot
from .slicer import Slicer
from .tools import axis_label, edges_to_centers

# Other imports
import numpy as np
import copy as cp
import matplotlib.pyplot as plt
import ipywidgets as widgets
import IPython.display as disp


def plot_1d(input_data, backend=None, logx=False, logy=False, logxy=False,
            color=None, filename=None, axes=None):
    """
    Plot a 1D spectrum.

    Input is a Dataset containing one or more Variables.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.
    If the data contains more than one dimensions, sliders are added.

    """

    if axes is None:
        axes = input_data.dims

    layout = dict(logx=logx or logxy, logy=logy or logxy)

    sv = Slicer1d(input_data=input_data, layout=layout, axes=axes, color=color)

    render_plot(figure=sv.fig, widgets=sv.box, filename=filename)

    return sv.members


class Slicer1d(Slicer):

    def __init__(self, input_data, layout, axes, color):

        super().__init__(input_data=input_data, axes=axes,
                         button_options=['X'])

        self.color = color
        self.fig, self.ax = plt.subplots(1, 1)
        self.members.update({"lines": {}, "error_x": {}, "error_y": {},
                             "error_xy": {}})
        self.color = color
        self.names = []
        ymin = 1.0e30
        ymax = -1.0e30
        for i, (name, var) in enumerate(sorted(self.input_data)):
            self.names.append(name)
            if var.variances is not None:
                err = np.sqrt(var.variances)
            else:
                err = 0.0

            if layout["logy"]:
                ymin = min(ymin, np.nanmin(np.log10(var.values - err)))
                ymax = max(ymax, np.nanmax(np.log10(var.values + err)))
            else:
                ymin = min(ymin, np.nanmin(var.values - err))
                ymax = max(ymax, np.nanmax(var.values + err))
            ylab = axis_label(var=var, name="")

        dy = 0.05*(ymax - ymin)
        layout["yrange"] = [ymin-dy, ymax+dy]
        if layout["logy"]:
            layout["yrange"] = 10.0**np.array(layout["yrange"])
        self.ax.set_ylim(layout["yrange"])
        if layout["logx"]:
            self.ax.set_xscale("log")
        if layout["logy"]:
            self.ax.set_yscale("log")

        # Disable buttons
        for key, button in self.buttons.items():
            if self.slider[key].disabled:
                button.disabled = True
        self.update_axes(str(list(self.slider_dims.keys())[-1]))

        self.ax.set_ylabel(ylab)
        self.ax.legend()

        self.keep_buttons = dict()
        if self.ndim > 1:
            self.make_keep_button()

        # vbox contains the original sliders and buttons. In mbox, we include
        # the keep trace buttons.
        self.mbox = self.vbox.copy()
        for key, val in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(val))
        self.box = widgets.VBox(self.mbox)
        self.box.layout.align_items = 'center'
        # print(self.box.children)

        # Populate the members
        self.members["fig"] = self.fig
        self.members["ax"] = self.ax

        return

    def make_keep_button(self):
        drop = widgets.Dropdown(options=self.names, description='',
                                layout={'width': 'initial'})
        but = widgets.Button(description="Keep", disabled=False,
                             button_style="", layout={'width': "70px"})
        # Generate a random color. TODO: should we initialise the seed?
        col = widgets.ColorPicker(
            concise=True, description='',
            value='#%02X%02X%02X' % (tuple(np.random.randint(0, 255, 3))),
            disabled=False)
        # Make a unique id
        key = str(id(but))
        setattr(but, "id", key)
        setattr(col, "id", key)
        but.on_click(self.keep_remove_trace)
        col.observe(self.update_trace_color, names="value")
        self.keep_buttons[key] = [drop, but, col]
        return

    def update_buttons(self, owner, event, dummy):
        for key, button in self.buttons.items():
            if key == owner.dim_str:
                self.slider[key].disabled = True
                button.disabled = True
                self.button_axis_to_dim["x"] = key
            else:
                self.slider[key].disabled = False
                button.value = None
                button.disabled = False
        self.update_axes(owner.dim_str)
        self.keep_buttons = dict()
        self.make_keep_button()
        self.mbox = self.vbox.copy()
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(b))
        self.box.children = tuple(self.mbox)
        return

    def update_axes(self, dim_str):
        self.ax.lines = []
        self.ax.collections = []
        self.members.update({"lines": {}, "error_x": {}, "error_y": {},
                             "error_xy": {}})

        new_x = self.slider_x[dim_str].values
        xc = edges_to_centers(new_x)

        for i, (name, var) in enumerate(sorted(self.input_data)):
            vslice = self.slice_data(var)
            if self.histograms[name][dim_str]:
                [self.members["lines"][name]] = self.ax.step(
                    new_x, np.concatenate(([0], vslice.values)), label=name,
                    color=self.color[i], zorder=10)
            else:
                [self.members["lines"][name]] = self.ax.plot(
                    new_x, vslice.values, label=name, color=self.color[i],
                    zorder=10)
            if var.variances is not None:
                if self.histograms[name][dim_str]:
                    self.members["error_y"][name] = self.ax.errorbar(xc, vslice.values, yerr=np.sqrt(vslice.variances),
                                                color=self.color[i], zorder=10, fmt="none")
                else:
                    self.members["error_y"][name] = self.ax.errorbar(new_x, vslice.values, yerr=np.sqrt(vslice.variances),
                                                color=self.color[i], zorder=10, fmt="none")

        self.ax.set_xlim([new_x[0], new_x[-1]])
        self.ax.set_xlabel(axis_label(self.slider_x[dim_str],
                                      name=self.slider_labels[dim_str]))
        return

    def slice_data(self, var):
        vslice = var
        # Slice along dimensions with active sliders
        for key, val in self.slider.items():
            if not val.disabled:
                vslice = vslice[val.dim, val.value]
        return vslice

    # Define function to update slices
    def update_slice(self, change):
        for i, (name, var) in enumerate(sorted(self.input_data)):
            vslice = self.slice_data(var)
            vals = vslice.values
            if self.histograms[name][self.button_axis_to_dim["x"]]:
                vals = np.concatenate(([0], vals))
            self.members["lines"][name].set_ydata(vals)
            if var.variances is not None:
                coll = self.members["error_y"][name].get_children()[0]
                coll.set_segments(
                    self.change_segments_y(
                        coll.get_segments(),
                        vslice.values,
                        np.sqrt(vslice.variances)))
        return

    def keep_remove_trace(self, owner):
        if owner.description == "Keep":
            self.keep_trace(owner)
        elif owner.description == "Remove":
            self.remove_trace(owner)
        return

    def keep_trace(self, owner):
        lab = self.keep_buttons[owner.id][0].value
        self.ax.lines.append(cp.copy(self.members["lines"][lab]))
        self.ax.lines[-1].set_color(self.keep_buttons[owner.id][2].value)
        self.ax.lines[-1].set_url(owner.id)
        self.ax.lines[-1].set_zorder(1)
        if self.input_data[lab].variances is not None:
            err = self.members["error_y"][lab].get_children()
            self.ax.collections.append(cp.copy(err[0]))
            self.ax.collections[-1].set_color(self.keep_buttons[owner.id][2].value)
            self.ax.collections[-1].set_url(owner.id)
            self.ax.collections[-1].set_zorder(1)

        for key, val in self.slider.items():
            if not val.disabled:
                lab = "{},{}:{}".format(lab, key, val.value)
        self.keep_buttons[owner.id][0] = widgets.Label(
            value=lab, layout={'width': "initial"}, title=lab)
        self.make_keep_button()
        owner.description = "Remove"
        self.mbox = self.vbox.copy()
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(b))
        self.box.children = tuple(self.mbox)
        return

    def remove_trace(self, owner):
        del self.keep_buttons[owner.id]
        lines = []
        for line in self.ax.lines:
            if line.get_url() != owner.id:
                lines.append(line)
        collections = []
        for coll in self.ax.collections:
            if coll.get_url() != owner.id:
                collections.append(coll)
        self.ax.lines = lines
        self.ax.collections = collections
        self.fig.canvas.draw()
        self.mbox = self.vbox.copy()
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(b))
        self.box.children = tuple(self.mbox)
        return

    def update_trace_color(self, change):
        for line in self.ax.lines:
            if line.get_url() == change["owner"].id:
                line.set_color(change["new"])
        for coll in self.ax.collections:
            if coll.get_url() == change["owner"].id:
                coll.set_color(change["new"])
        self.fig.canvas.draw()
        return

    def generate_new_segments(self, x, y):
        arr1 = np.array([x, x]).T.flatten()
        arr2 = np.array([y, y]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(x), 2, 2)

    def change_segments_y(self, s, y, e):
        # TODO: this can be optimized to substitute the y values in-place in
        # the original segments array
        arr1 = np.array(s).flatten()[::2]
        arr2 = np.array([y-np.sqrt(e), y+np.sqrt(e)]).T.flatten()
        return np.array([arr1, arr2]).T.flatten().reshape(len(y), 2, 2)
