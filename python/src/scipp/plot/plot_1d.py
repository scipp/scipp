# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .render import render_plot
from .slicer import Slicer
from .tools import axis_label

# Other imports
import numpy as np
import plotly.graph_objs as go
import ipywidgets as widgets


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

    ymin = 1.0e30
    ymax = -1.0e30

    data = []
    for i, (name, var) in enumerate(sorted(input_data)):

        ax = var.dims
        if var.variances is not None:
            err = np.sqrt(var.variances)
        else:
            err = 0.0

        ymin = min(ymin, np.nanmin(var.values - err))
        ymax = max(ymax, np.nanmax(var.values + err))

        # Define trace
        trace = dict(name=name, type="scattergl")
        if color is not None:
            trace["marker"] = {"color": color[i]}
        data.append(trace)

    if axes is None:
        axes = ax

    layout = dict(
        xaxis=dict(),
        yaxis=dict(),
        showlegend=True,
        legend=dict(x=0.0, y=1.15, orientation="h"),
        height=config.height,
        width=config.width
    )
    if logx or logxy:
        layout["xaxis"]["type"] = "log"
    if logy or logxy:
        layout["yaxis"]["type"] = "log"
        [ymin, ymax] = np.log10([ymin, ymax])
    dy = 0.05*(ymax - ymin)
    layout["yaxis"]["range"] = [ymin-dy, ymax+dy]

    sv = Slicer1d(data=data, layout=layout, input_data=input_data, axes=axes,
                  color=color)
    render_plot(static_fig=sv.fig, interactive_fig=sv.box, backend=backend,
                filename=filename)

    return


class Slicer1d(Slicer):

    def __init__(self, data, layout, input_data, axes, color):

        super().__init__(input_data=input_data, axes=axes,
                         button_options=['X'])

        self.color = color
        self.fig = go.FigureWidget(layout=layout)

        self.traces = dict()
        for i, (name, var) in enumerate(sorted(self.input_data)):
            trace = dict(name=name, type="scattergl")
            if color is not None:
                trace["marker"] = {"color": color[i]}
            self.traces[name] = i
            self.fig.add_trace(trace)

        # Disable buttons
        for key, button in self.buttons.items():
            if self.slider[key].disabled:
                button.disabled = True
        self.update_axes(str(self.slider_dims[-1]))
        self.update_slice(None)
        self.update_histograms()

        self.keep_buttons = dict()
        if self.ndim > 1:
            self.make_keep_button()

        # vbox contains the original sliders and buttons. In mbox, we include
        # the keep trace buttons.
        self.mbox = [self.fig] + self.vbox
        for key, val in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(val))
        self.box = widgets.VBox(self.mbox)
        self.box.layout.align_items = 'center'

        return

    def make_keep_button(self):
        drop = widgets.Dropdown(options=self.traces.keys(), description='',
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
            else:
                self.slider[key].disabled = False
                button.value = None
                button.disabled = False
        self.update_axes(owner.dim_str)
        self.update_slice(None)
        self.update_histograms()

        self.keep_buttons = dict()
        self.make_keep_button()
        self.mbox = [self.fig] + self.vbox
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(b))
        self.box.children = tuple(self.mbox)
        return

    def update_axes(self, dim_str):
        self.fig.data = self.fig.data[:len(self.input_data)]
        self.fig.update_traces(x=self.slider_x[dim_str].values)
        self.fig.layout["xaxis"]["title"] = axis_label(
            self.slider_x[dim_str], name=self.slider_labels[dim_str])
        return

    # Define function to update slices
    def update_slice(self, change):
        # The dimensions to be sliced have been saved in slider_dims
        for i, (name, var) in enumerate(sorted(self.input_data)):
            vslice = var
            # Slice along dimensions with active sliders
            for key, val in self.slider.items():
                if not val.disabled:
                    if i == 0:
                        self.lab[key].value = self.make_slider_label(
                            self.slider_x[key], val.value)
                    vslice = vslice[val.dim, val.value]
            self.fig.data[i].y = vslice.values
            if var.variances is not None:
                self.fig.data[i]["error_y"].array = np.sqrt(vslice.variances)
        return

    def update_histograms(self):
        for i in range(len(self.fig.data)):
            trace = self.fig.data[i]
            if len(trace.x) == len(trace.y) + 1:
                trace["line"] = {"shape": "hvh"}
                trace["x"] = 0.5 * (trace["x"][:-1] + trace["x"][1:])
                trace["fill"] = "tozeroy"
                trace["mode"] = "lines"
            else:
                trace["line"] = None
                trace["fill"] = None
                trace["mode"] = None
        return

    def keep_remove_trace(self, owner):
        if owner.description == "Keep":
            self.keep_trace(owner)
        elif owner.description == "Remove":
            self.remove_trace(owner)
        return

    def keep_trace(self, owner):
        lab = self.keep_buttons[owner.id][0].value
        self.fig.add_trace(self.fig.data[self.traces[lab]])
        self.fig.data[-1]["marker"]["color"] = self.keep_buttons[
            owner.id][2].value
        self.fig.data[-1]["showlegend"] = False
        self.fig.data[-1]["meta"] = owner.id
        for key, val in self.slider.items():
            if not val.disabled:
                lab = "{},{}:{}".format(lab, key, val.value)
        self.keep_buttons[owner.id][0] = widgets.Label(
            value=lab, layout={'width': "initial"}, title=lab)
        self.make_keep_button()
        owner.description = "Remove"
        self.mbox = [self.fig] + self.vbox
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(b))
        self.box.children = tuple(self.mbox)
        return

    def remove_trace(self, owner):
        del self.keep_buttons[owner.id]
        data = []
        for tr in self.fig.data:
            if tr.meta != owner.id:
                data.append(tr)
        self.fig.data = data
        self.mbox = [self.fig] + self.vbox
        for k, b in self.keep_buttons.items():
            self.mbox.append(widgets.HBox(b))
        self.box.children = tuple(self.mbox)
        return

    def update_trace_color(self, change):
        for tr in self.fig.data:
            if tr.meta == change["owner"].id:
                tr["marker"]["color"] = change["new"]
                return
