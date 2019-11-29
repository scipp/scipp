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
# import plotly.graph_objs as go
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

    TODO: find a more general way of handling arguments to be sent to plotly,
    probably via a dictionay of arguments
    """

    # ymin = 1.0e30
    # ymax = -1.0e30

    # data = []
    # for i, (name, var) in enumerate(sorted(input_data)):

    #     ax = var.dims
    #     if var.variances is not None:
    #         err = np.sqrt(var.variances)
    #     else:
    #         err = 0.0

    #     ymin = min(ymin, np.nanmin(var.values - err))
    #     ymax = max(ymax, np.nanmax(var.values + err))
    #     ylab = axis_label(var=var, name="")

    #     # # Define trace
    #     # trace = dict(name=name, type="scattergl")
    #     # if color is not None:
    #     #     trace["marker"] = {"color": color[i]}
    #     # data.append(trace)

    if axes is None:
        axes = input_data.dims

    layout = dict(logx=logx or logxy, logy=logy or logxy)

    # # layout = dict(
    # #     xaxis=dict(),
    # #     yaxis=dict(),
    # #     showlegend=True,
    # #     legend=dict(x=0.0, y=1.15, orientation="h"),
    # #     height=config.height,
    # #     width=config.width
    # # )
    # # if logx or logxy:
    # #     layout["xaxis"]["type"] = "log"
    # if logy or logxy:
    #     # layout["yaxis"]["type"] = "log"
    #     [ymin, ymax] = np.log10([ymin, ymax])
    # dy = 0.05*(ymax - ymin)
    # layout["yrange"] = [ymin-dy, ymax+dy]
    # if logy or logxy:
    #     layout["yrange"] = 10.0**np.array(layout["yrange"])

    # #     layout["yrange"] = [10.0**(ymin-dy), 10.0**(ymax+dy)]
    # # else:
    # #     dy = 0.05*(ymax - ymin)
    # #     layout["yrange"] = [ymin-dy, ymax+dy]


    sv = Slicer1d(input_data=input_data, layout=layout, axes=axes, color=color)
    disp.display(sv.box)

    # render_plot(static_fig=sv.fig, interactive_fig=sv.box, backend=backend,
    #             filename=filename)

    return


class Slicer1d(Slicer):

    def __init__(self, input_data, layout, axes, color):

        super().__init__(input_data=input_data, axes=axes,
                         button_options=['X'])

        self.color = color
        self.fig, self.ax = plt.subplots(1, 1)
        

        self.traces = dict(lines=dict(), error_x=dict(), error_y=dict(), error_xy=dict())
        self.color = color
        # # self.trace_types = dict()
        self.names = []
        ymin = 1.0e30
        ymax = -1.0e30
        for i, (name, var) in enumerate(sorted(self.input_data)):
            self.names.append(name)
            if var.variances is not None:
                err = np.sqrt(var.variances)
            else:
                err = 0.0

            ymin = min(ymin, np.nanmin(var.values - err))
            ymax = max(ymax, np.nanmax(var.values + err))
            ylab = axis_label(var=var, name="")
        #     # trace = dict(name=name, type="scattergl")
        #     # if color is not None:
        #     #     trace["marker"] = {"color": color[i]}
        #     # self.traces[name] = dict(index=i)
        #     # line=self.ax.plot([0], [0], label=name))

        #     # self.trace_types[name] = "lines"
        #     [self.traces["lines"][name]] = self.ax.plot([1], [1], label=name, color=color[i], zorder=10)

        #     if var.variances is not None:
        #         # self.trace_types[name] = "error_y"
        #         self.traces["error_y"][name] = self.ax.errorbar([1], [1], yerr=[0],
        #                                         color=color[i], zorder=10, fmt="none")
        #     # else:
        #         # self.trace_types[name] = "lines"
        #         # self.traces["lines"][name] = self.ax.plot([1], [1], label=name, color=color[i], zorder=10)
        # self.lines = self.ax.lines.copy()
        # self.collections = self.ax.collections.copy()

        if layout["logy"]:
            [ymin, ymax] = np.log10([ymin, ymax])
        dy = 0.05*(ymax - ymin)
        layout["yrange"] = [ymin-dy, ymax+dy]
        if layout["logy"]:
            layout["yrange"] = 10.0**np.array(layout["yrange"])
        self.ax.set_ylim(layout["yrange"])
        if layout["logx"]:
            self.ax.set_xscale("log")
        if layout["logy"]:
            self.ax.set_yscale("log")
        # self.ax.set_ylabel(ylab)
        # self.ax.legend()

        # Disable buttons
        for key, button in self.buttons.items():
            if self.slider[key].disabled:
                button.disabled = True
        self.update_axes(str(list(self.slider_dims.keys())[-1]))
        # self.update_slice(None)
        # self.update_histograms()
        self.ax.set_ylabel(ylab)
        self.ax.legend()

        self.keep_buttons = dict()
        if self.ndim > 1:
            self.make_keep_button()

        # vbox contains the original sliders and buttons. In mbox, we include
        # the keep trace buttons.
        # self.mbox = [self.fig] + self.vbox
        self.mbox = self.vbox.copy()
        for key, val in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(val))
        self.box = widgets.VBox(self.mbox)
        self.box.layout.align_items = 'center'

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
        # self.update_slice(None)
        # self.update_histograms()

        self.keep_buttons = dict()
        self.make_keep_button()
        # self.mbox = [self.fig] + self.vbox
        self.mbox = self.vbox.copy()
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(b))
        self.box.children = tuple(self.mbox)
        return

    def update_axes(self, dim_str):
        # # print(self.ax.lines)
        # # newlist = self.ax.lines[:len(self.input_data)]
        # # self.ax.lines = newlist
        # # print(self.ax.lines)
        # # print(self.lines)
        # # print(self.ax.collections)
        # # print(self.collections)
        self.ax.lines = []
        self.ax.collections = []
        self.traces = dict(lines=dict(), error_x=dict(), error_y=dict(), error_xy=dict())

        new_x = self.slider_x[dim_str].values
        xc = edges_to_centers(new_x)

        for i, (name, var) in enumerate(sorted(self.input_data)):
            vslice = self.slice_data(var)
            if self.histograms[name][dim_str]:
                [self.traces["lines"][name]] = self.ax.step(
                    new_x, np.concatenate(([0], vslice.values)), label=name,
                    color=self.color[i], zorder=10)
            else:
                [self.traces["lines"][name]] = self.ax.plot(
                    new_x, vslice.values, label=name, color=self.color[i],
                    zorder=10)
            if var.variances is not None:
                if self.histograms[name][dim_str]:
                    self.traces["error_y"][name] = self.ax.errorbar(xc, vslice.values, yerr=np.sqrt(vslice.variances),
                                                color=self.color[i], zorder=10, fmt="none")
                else:
                    self.traces["error_y"][name] = self.ax.errorbar(new_x, vslice.values, yerr=np.sqrt(vslice.variances),
                                                color=self.color[i], zorder=10, fmt="none")




        #     self.traces["lines"][name].set_xdata(new_x)
        #     if var.variances is not None:
        #         if self.histograms[name][dim_str]:
        #             segs = self.generate_new_segments(xc, np.ones_like(xc))
        #         else:
        #             segs = self.generate_new_segments(new_x, np.ones_like(new_x))
        #         self.traces["error_y"][name].get_children()[0].set_segments(segs)


        # # for line in self.ax.lines:
        # #     line.set_xdata(new_x)

        # # for coll in self.collections:
        # #     coll.set_segments(self.generate_segments(new_x, np.ones_like(new_x), np.zeros_like(new_x)))

        # # # for key, val in self.traces["error_y"].items():
        # # #     # print(val)
        # # #     # print(val[0], val[1], val[2])
        # # #     val.get_children()[1].set_segments(self.generate_segments(new_x, np.ones_like(new_x), np.zeros_like(new_x)))


        # # # # for i, line in enumerate(self.ax.lines):
        # # # #     if i >= len(self.input_data):
        # # # #         self.ax.lines.remove(
        # # # # self.fig.data = self.fig.data[:len(self.input_data)]
        # # # for i, (name, var) in enumerate(sorted(self.input_data)):
        # # #     # trace = dict(name=name, type="scattergl")
        # # #     # if color is not None:
        # # #     #     trace["marker"] = {"color": color[i]}
        # # #     self.traces[name] = dict(index=i)
        # # #     # line=self.ax.plot([0], [0], label=name))
        # # #     if var.variances is not None:
        # # #         self.traces[name]["errorbar"] = self.ax.errorbar([0], [0], yerr=[0],
        # # #                                         color=color[i])
        # # #     else:
        # # #         self.traces[name]["line"] = self.ax.plot([0], [0], label=name, color=color[i])

        # for line in self.ax.lines:
        #     line.set_xdata(self.slider_x[dim_str].values)
        #     # print(line.get_xdata())
        self.ax.set_xlim([new_x[0], new_x[-1]])
        self.ax.set_xlabel(axis_label(self.slider_x[dim_str],
                                      name=self.slider_labels[dim_str]))
        # self.fig.update_traces(x=self.slider_x[dim_str].values)
        # self.fig.layout["xaxis"]["title"] = axis_label(
        #     self.slider_x[dim_str], name=self.slider_labels[dim_str])
        return

    def slice_data(self, var):
        vslice = var
        # Slice along dimensions with active sliders
        for key, val in self.slider.items():
            if not val.disabled:
                vslice = vslice[val.dim, val.value]
        return vslice
        # vals = vslice.values
        # if self.histograms[name][key]:
        #     vals = np.concatenate(([0], vals))

        # self.traces["lines"][name].set_ydata(vals)
        # if var.variances is not None:
        #     # self.traces["error_y"][name][0].set_ydata(vslice.values)
        #     coll = self.traces["error_y"][name].get_children()[0]
        #     coll.set_segments(
        #         self.change_segments_y(
        #             coll.get_segments(),
        #             vslice.values,
        #             np.sqrt(vslice.variances)))

    # Define function to update slices
    def update_slice(self, change):
        # The dimensions to be sliced have been saved in slider_dims
        # print(change)
        # print(self.button_axis_to_dim)
        for i, (name, var) in enumerate(sorted(self.input_data)):
            vslice = self.slice_data(var)
            # # Slice along dimensions with active sliders
            # for key, val in self.slider.items():
            #     if not val.disabled:
            #         if i == 0:
            #             self.lab[key].value = self.make_slider_label(
            #                 self.slider_x[key], val.value)
            #         vslice = vslice[val.dim, val.value]
            # # # self.fig.data[i].y = vslice.values
            # # self.ax.lines[i].set_ydata(vslice.values)
            vals = vslice.values
            if self.histograms[name][self.button_axis_to_dim["x"]]:
                vals = np.concatenate(([0], vals))
            self.traces["lines"][name].set_ydata(vals)
            if var.variances is not None:
                # self.traces["error_y"][name][0].set_ydata(vslice.values)
                coll = self.traces["error_y"][name].get_children()[0]
                coll.set_segments(
                    self.change_segments_y(
                        coll.get_segments(),
                        vslice.values,
                        np.sqrt(vslice.variances)))
            #     # self.fig.data[i]["error_y"].array = np.sqrt(vslice.variances)
            # else:
            #     self.traces["lines"][name][0].set_ydata(vslice.values)
        return

    def update_histograms(self):
        # for i in range(len(self.fig.data)):
        #     trace = self.fig.data[i]
        #     if len(trace.x) == len(trace.y) + 1:
        #         trace["line"] = {"shape": "hvh"}
        #         trace["x"] = 0.5 * (trace["x"][:-1] + trace["x"][1:])
        #         trace["fill"] = "tozeroy"
        #         trace["mode"] = "lines"
        #     else:
        #         trace["line"] = None
        #         trace["fill"] = None
        #         trace["mode"] = None
        return

    def keep_remove_trace(self, owner):
        if owner.description == "Keep":
            self.keep_trace(owner)
        elif owner.description == "Remove":
            self.remove_trace(owner)
        return

    def keep_trace(self, owner):
        lab = self.keep_buttons[owner.id][0].value
        # if self.trace_types[lab] == "lines":
        #     # line = self.traces["lines"][name]
        #     # self.ax.plot(line.get_xdata(), line.get_ydata(), color=self.keep_buttons[owner.id][2].value)
        #     # # if var.variances is not None:
        #     # #     self.trace_types[name] = "error_y"
        #     # #     self.traces["error_y"][name] = self.ax.errorbar([1], [1], yerr=[0],
        #     # #                                     color=color[i])
        #     # # else:
        #     # #     self.trace_types[name] = "lines"
        #     # #     self.traces["lines"][name] = self.ax.plot([1], [1], label=name, color=color[i])
        #     self.ax.lines.append(cp.copy(self.traces["lines"][lab]))
        #     # self.ax.lines[-1].set_color(self.keep_buttons[owner.id][2].value)
        #     # self.ax.lines[-1].set_url(owner.id)

        self.ax.lines.append(cp.copy(self.traces["lines"][lab]))
        self.ax.lines[-1].set_color(self.keep_buttons[owner.id][2].value)
        self.ax.lines[-1].set_url(owner.id)
        self.ax.lines[-1].set_zorder(1)
        if self.input_data[lab].variances is not None:
        # if self.trace_types[lab] == "error_y":
            err = self.traces["error_y"][lab].get_children()

            # self.ax.plot(line.get_xdata(), line.get_ydata(), color=color[i], self.keep_buttons[owner.id][2].value)

            # self.ax.lines.append(cp.copy(err[0]))
            self.ax.collections.append(cp.copy(err[0]))
            self.ax.collections[-1].set_color(self.keep_buttons[owner.id][2].value)
            self.ax.collections[-1].set_url(owner.id)
            self.ax.collections[-1].set_zorder(1)






        # # self.fig.add_trace(self.fig.data[self.traces[lab]])
        # self.fig.data[-1]["marker"]["color"] = self.keep_buttons[
        #     owner.id][2].value
        # self.fig.data[-1]["showlegend"] = False
        # self.fig.data[-1]["meta"] = owner.id
        for key, val in self.slider.items():
            if not val.disabled:
                lab = "{},{}:{}".format(lab, key, val.value)
        self.keep_buttons[owner.id][0] = widgets.Label(
            value=lab, layout={'width': "initial"}, title=lab)
        self.make_keep_button()
        owner.description = "Remove"
        # self.mbox = [self.fig] + self.vbox
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

        # for tr in self.fig.data:
        #     if tr.meta != owner.id:
        #         data.append(tr)
        # self.fig.data = data
        # self.mbox = [self.fig] + self.vbox
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

