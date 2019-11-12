# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .render import render_plot
from .tools import edges_to_centers, get_1d_axes
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

    Input is a dictionary containing a list of DataProxy.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.

    TODO: find a more general way of handling arguments to be sent to plotly,
    probably via a dictionay of arguments
    """

    ymin = 1.0e30
    ymax = -1.0e30

    data = []
    for i, (name, var) in enumerate(sorted(input_data)):

        # xlab, ylab, x, y = get_1d_axes(var, axes, name)
        ax = var.dims
        if var.variances is not None:
            err = np.sqrt(var.variances)
        else:
            err = 0.0

        ymin = min(ymin, np.nanmin(var.values - err))
        ymax = max(ymax, np.nanmax(var.values + err))

        # nx = x.shape[0]
        # ny = y.shape[0]
        # histogram = False
        # if nx == ny + 1:
        #     histogram = True

        # Define trace
        # trace = dict(x=x, y=y, name=name, type="scattergl")
        trace = dict(name=name, type="scattergl")
        # if histogram:
        #     trace["line"] = {"shape": "hv"}
        #     trace["y"] = np.concatenate((trace["y"], [0.0]))
        #     trace["fill"] = "tozeroy"
        #     trace["mode"] = "lines"
        # print(color)
        # print(color[i])
        if color is not None:
            trace["marker"] = {"color": color[i]}
        # # Include variance if present
        # if var.variances is not None:
        #     err_dict = dict(
        #             type="data",
        #             array=np.sqrt(var.variances),
        #             visible=True,
        #             color=color[i])
        #     if histogram:
        #         trace2 = dict(x=edges_to_centers(x), y=y, showlegend=False,
        #                       type="scattergl", mode="markers",
        #                       error_y=err_dict,
        #                       marker={"color": color[i]})
        #         data.append(trace2)
        #     else:
        #         trace["error_y"] = err_dict

        data.append(trace)

    if axes is None:
        axes = ax

    dy = 0.05*(ymax - ymin)
    layout = dict(
        yaxis=dict(range=[ymin-dy, ymax+dy]),
        showlegend=True,
        legend=dict(x=0.0, y=1.15, orientation="h"),
        height=config.height,
        width=config.width
    )
    # if histogram:
    #     layout["barmode"] = "overlay"
    if logx or logxy:
        layout["xaxis"]["type"] = "log"
    if logy or logxy:
        layout["yaxis"]["type"] = "log"

    sv = Slicer1d(data=data, layout=layout, input_data=input_data, axes=axes, color=color)


    render_plot(static_fig=sv.fig, interactive_fig=sv.vbox, backend=backend,
                filename=filename)

    # fig = go.Figure(data=data, layout=layout)
    # render_plot(static_fig=fig, interactive_fig=fig, backend=backend,
    #             filename=filename)
    return



class Slicer1d(Slicer):

    def __init__(self, data, layout, input_data, axes, color):

        super().__init__(input_data=input_data, axes=axes, button_options=['X'])

        self.color = color

        self.fig = go.FigureWidget(data=data, layout=layout)

        # Disable buttons
        for key, button in self.buttons.items():
            if self.slider[key].disabled:
                button.disabled = True
        self.update_axes(str(axes[-1]))
        self.update_slice(None)
        self.update_histograms()
        self.vbox = [self.fig] + self.vbox
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'

        return



    def update_buttons(self, owner, event, dummy):

        # toggle_slider = False
        # if not self.slider[owner.dim_str].disabled:
        #     toggle_slider = True
        #     self.slider[owner.dim_str].disabled = True
        for key, button in self.buttons.items():
            # if (button.value == owner.value) and (key != owner.dim_str):
            if key == owner.dim_str:
                self.slider[key].disabled = True
                button.disabled = True
            else:
                self.slider[key].disabled = False
                button.value = None
                button.disabled = False
        #         if self.slider[key].disabled:
        #             button.value = owner.old_value
        #         else:
        #             button.value = None
        #         button.old_value = button.value
        #         if toggle_slider:
        #             self.slider[key].disabled = False
        # owner.old_value = owner.value
        self.update_axes(owner.dim_str)
        self.update_slice(None)
        self.update_histograms()
        # for i in range(len(self.fig.data)):
        #     histogram = False
        #     trace = self.fig.data[i]
        #     if len(trace.x) == len(trace.y) + 1:
        #         histogram = True
        #     if histogram:
        #         trace["line"] = {"shape": "hv"}
        #         trace["y"] = np.concatenate((trace["y"], [0.0]))
        #         trace["fill"] = "tozeroy"
        #         trace["mode"] = "lines"
        #     else:
        #         trace["line"] = None
        #         # trace["y"] = np.concatenate((trace["y"], [0.0]))
        #         trace["fill"] = None
        #         trace["mode"] = None

        return

    def update_axes(self, dim_str):
        for i in range(len(self.fig.data)):
            self.fig.data[i].x = self.slider_x[dim_str].values

        # # Go through the buttons and select the right coordinates for the axes
        # for key, button in self.buttons.items():
        #     if self.slider[key].disabled:
        #         but_val = button.value.lower()
        #         # for i in range(1 + self.show_variances):
        #         #     if self.rasterize:
        #         #         self.fig.data[i][but_val] = \
        #         #             self.slider_x[key].values[[0, -1]]
        #         #     else:
        #         self.fig.data[i]["x"] = self.slider_x[key].values
        #         # if self.surface3d:
        #         #     self.fig.layout.scene1["{}axis_title".format(
        #         #         but_val)] = axis_label(self.slider_x[key],
        #         #                                name=self.slider_labels[key])
        #         #     if self.show_variances:
        #         #         self.fig.layout.scene2["{}axis_title".format(
        #         #             but_val)] = axis_label(
        #         #                 self.slider_x[key],
        #         #                 name=self.slider_labels[key])
        #         # else:
        #         #     if self.show_variances:
        #         #         func = getattr(self.fig, 'update_{}axes'.format(
        #         #             but_val))
        #         #         for i in range(2):
        #         #             func(title_text=axis_label(
        #         #                 self.slider_x[key],
        #         #                 name=self.slider_labels[key]),
        #         #                 row=1, col=i+1)
        #         #     else:
        #         # axis_str = "{}axis".format(but_val)
        self.fig.layout["xaxis"]["title"] = axis_label(
            self.slider_x[dim_str], name=self.slider_labels[dim_str])

        return

    # Define function to update slices
    def update_slice(self, change):
        # The dimensions to be sliced have been saved in slider_dims
        for i, (name, var) in enumerate(sorted(self.input_data)):
            # print(name)
            vslice = var
            # Slice along dimensions with active sliders
            # button_dims = [None, None]
            # print("start slicing")
            for key, val in self.slider.items():
                # print(key, val)
                if not val.disabled:
                    if i == 0:
                        self.lab[key].value = str(
                            self.slider_x[key].values[val.value])
                    # print("slicing along", val.dim, val.value)
                    vslice = vslice[val.dim, val.value]
                # else:
                #     button_dims[self.buttons[key].value.lower() == "y"] = val.dim

            self.fig.data[i].y = vslice.values
            # nx = len(self.fig.data[i].y)
            # ny = y.shape[0]
            # histogram = False
            # if nx == ny + 1:
            #     histogram = True
            if var.variances is not None:
                self.fig.data[i]["error_y"].array = np.sqrt(vslice.variances)

        # # Check if dimensions of arrays agree, if not, plot the transpose
        # slice_dims = vslice.dims
        # transp = slice_dims == button_dims
        # self.update_z2d(vslice.values, transp, self.cb["log"], 0)
        # if self.show_variances:
        #     self.update_z2d(vslice.variances, transp, self.cb["log"], 1)
        return

    def update_histograms(self):
        for i in range(len(self.fig.data)):
            histogram = False
            trace = self.fig.data[i]
            if len(trace.x) == len(trace.y) + 1:
            #     histogram = True
            # if histogram:
                trace["line"] = {"shape": "hvh"}
                trace["x"] = 0.5 * (trace["x"][:-1] + trace["x"][1:])
                # trace["x"] = np.concatenate(([trace["x"][0]-], 0.5 * (trace["x"][:-1] + trace["x"][1:]), [trace["x"][-1]]))
                # trace["y"] = np.concatenate(([0.0], trace["y"], [0.0]))
                # print(trace["error_y"].array)
                # if trace["error_y"].array is not None:
                #     trace["error_y"].array = np.concatenate(([0.0], trace["error_y"].array, [0.0]))
                trace["fill"] = "tozeroy"
                trace["mode"] = "lines"
            else:
                trace["line"] = None
                # trace["y"] = np.concatenate((trace["y"], [0.0]))
                trace["fill"] = None
                trace["mode"] = None
        return
