# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

import numpy as np
from collections import namedtuple
from .tools import edges_to_centers, axis_label, parse_colorbar, \
                   process_dimensions, get_coord_array

# Plotly imports
from IPython.display import display
from plotly.io import write_html, write_image
from plotly.colors import DEFAULT_PLOTLY_COLORS
import ipywidgets as widgets
import plotly.graph_objs as go
from plotly.subplots import make_subplots


def plot_plotly(input_data, ndim=0, name=None, config=None,
                collapse=None, projection="2d", **kwargs):
    """
    Function to automatically dispatch the input dataset to the appropriate
    plotting function depending on its dimensions
    """

    if ndim == 1:
        plot_1d(input_data, config=config, **kwargs)
    elif projection.lower() == "2d":
        # if waterfall is not None:
        #     plot_waterfall(input_data, name=name, dim=waterfall, config=config,
        #                    **kwargs)
        # else:
        plot_2d(input_data, name=name, config=config, ndim=ndim, **kwargs)
    elif projection.lower() == "3d":
        plot_3d(input_data, name=name, config=config, ndim=ndim, **kwargs)
    else:
        raise RuntimeError("Wrong projection type.")
    return


def plot_1d(input_data, logx=False, logy=False, logxy=False, axes=None,
            color=None, filename=None, config=None):
    """
    Plot a 1D spectrum.

    Input is a dictionary containing a list of DataProxy.
    If the coordinate of the x-axis contains bin edges, then a bar plot is
    made.

    TODO: find a more general way of handling arguments to be sent to plotly,
    probably via a dictionay of arguments
    """

    data = []
    color_count = 0
    for name, var in input_data.items():
        xcoord = var.coords[var.dims[0]]
        x = xcoord.values
        xlab = axis_label(xcoord)
        y = var.values
        ylab = axis_label(var=var, name=name)

        nx = x.shape[0]
        ny = y.shape[0]
        histogram = False
        if nx == ny + 1:
            histogram = True

        # Define trace
        trace = dict(x=x, y=y, name=ylab, type='scattergl')
        if histogram:
            trace["line"] = {"shape": 'hv'}
            trace["y"] = np.concatenate((trace["y"], [0.0]))
            trace["fill"] = 'tozeroy'
        if color is not None:
            trace["marker"] = {"color": color[color_count]}
        # Include variance if present
        if var.variances is not None:
            err_dict = dict(
                    type='data',
                    array=np.sqrt(var.variances),
                    visible=True,
                    color=color[color_count])
            if histogram:
                trace2 = dict(x=edges_to_centers(x), y=y, showlegend=False,
                    type='scattergl',
                mode='markers', error_y=err_dict, marker={"color": color[color_count]})
                data.append(trace2)
            else:
                trace["error_y"] = err_dict

        data.append(trace)
        color_count += 1

    layout = dict(
        xaxis=dict(title=xlab),
        yaxis=dict(),
        showlegend=True,
        legend=dict(x=0.0, y=1.15, orientation="h"),
        height=config.height
    )
    if histogram:
        layout["barmode"] = "overlay"
    if logx or logxy:
        layout["xaxis"]["type"] = "log"
    if logy or logxy:
        layout["yaxis"]["type"] = "log"

    fig = go.Figure(data=data, layout=layout)
    if filename is not None:
        if filename.endswith(".html"):
            write_html(fig=fig, file=filename, auto_open=False)
        else:
            write_image(fig=fig, file=filename)
    else:
        display(fig)
    return


def plot_2d(input_data, axes=None, contours=False, cb=None,
                     filename=None, name=None, config=None, figsize=None,
                     show_variances=False, ndim=0, **kwargs):
    """
    Plot a 2D slice through a N dimensional dataset. For every dimension above
    2, a slider is created to adjust the position of the slice in that
    particular dimension.
    """

    if axes is None:
        axes = input_data.dims

    if contours:
        plot_type = 'contour'
    else:
        plot_type = 'heatmap'

    # Parse colorbar
    cbar = parse_colorbar(config.cb, cb, plotly=True)

    # Make title
    title = axis_label(var=input_data, name=name, log=cbar["log"])

    if figsize is None:
        figsize = [config.width, config.height]

    layout = {"height": figsize[1], "width": figsize[0]}
    if input_data.variances is not None and show_variances:
        layout["height"] = 0.7 * layout["height"]

    data = dict(x=[0.0],
                y=[0.0],
                z=[0.0],
                type=plot_type,
                colorscale=cbar["name"],
                colorbar=dict(
                    title=title,
                    titleside='right',
                    lenmode='fraction',
                    len=1.05,
                    thicknessmode='fraction',
                    thickness=0.03)
                )

    sv = Slicer2d(data=data, layout=layout, input_data=input_data, axes=axes,
                  value_name=name, cb=cbar, show_variances=show_variances)

    if filename is not None:
        write_image(fig=sv.fig, file=filename)
    else:
        display(sv.vbox)
    return


class Slicer2d:

    def __init__(self, data, layout, input_data, axes,
                 value_name, cb, show_variances):

        self.input_data = input_data
        self.show_variances = ((self.input_data.variances is not None) and
                               show_variances)
        self.value_name = value_name
        self.cb = cb

        # Get the dimensions of the image to be displayed
        self.coords = self.input_data.coords
        self.shapes = dict(zip(self.input_data.dims, self.input_data.shape))

        # Size of the slider coordinate arrays
        self.slider_nx = dict()
        # Save dimensions tags for sliders, e.g. Dim.X
        self.slider_dims = []
        # Store coordinates of dimensions that will be in sliders
        self.slider_x = dict()
        for ax in axes:
            self.slider_dims.append(ax)
        self.ndim = len(self.slider_dims)
        for dim in self.slider_dims:
            key = str(dim)
            self.slider_nx[key] = self.shapes[dim]
            self.slider_x[key] = self.coords[dim].values
        self.nslices = len(self.slider_dims)

        # Initialise list for VBox container
        self.vbox = []

        # Initialise slider and label containers
        self.lab = dict()
        self.slider = dict()
        self.buttons = dict()
        # Default starting index for slider
        indx = 0

        # Now begin loop to construct sliders
        button_values = [None] * (self.ndim - 2) + ['Y'] + ['X']
        for i, dim in enumerate(self.slider_dims):
            key = str(dim)
            # Add a label widget to display the value of the z coordinate
            self.lab[key] = widgets.Label(value=str(self.slider_x[key][indx]))
            # Add an IntSlider to slide along the z dimension of the array
            self.slider[key] = widgets.IntSlider(
                value=indx,
                min=0,
                max=self.slider_nx[key] - 1,
                step=1,
                description=key,
                continuous_update=True,
                readout=False, disabled=(i>=self.ndim-2)
            )
            self.buttons[key] = widgets.ToggleButtons(
                options=['X', 'Y'], description='',
                value=button_values[i],
                disabled=False,
                button_style='')
            setattr(self.buttons[key], "dim_str", key)
            setattr(self.buttons[key], "dim", dim)
            setattr(self.buttons[key], "old_value", self.buttons[key].value)
            setattr(self.slider[key], "dim_str", key)
            setattr(self.slider[key], "dim", dim)
            self.buttons[key].on_msg(self.update_buttons)
            # Add an observer to the slider
            self.slider[key].observe(self.update_slice2d, names="value")
            # Add coordinate name and unit
            self.vbox.append(widgets.HBox([self.slider[key], self.lab[key], self.buttons[key]]))

        # Initialise Figure and VBox objects
        self.fig = None
        params = {"values": {"cbmin": "min", "cbmax": "max"},
                  "variances": None}
        if self.show_variances:
            params["variances"] = {"cbmin": "min_var", "cbmax": "max_var"}
            self.fig = go.FigureWidget(make_subplots(rows=1, cols=2, horizontal_spacing=0.16))
            data["colorbar"]["x"] = 0.42
            data["colorbar"]["thickness"] = 0.02
            self.fig.add_trace(data, row=1, col=1)
            data["colorbar"]["title"] = "variances"
            data["colorbar"]["x"] = 1.0
            self.fig.add_trace(data, row=1, col=2)
            self.fig.update_layout(height=layout["height"], width=layout["width"])
        else:
            self.fig = go.FigureWidget(data=[data], layout=layout)

        # Set colorbar limits once to keep them constant for slicer
        # TODO: should there be auto scaling as slider value is changed?
        for i, (key, val) in enumerate(sorted(params.items())):
            if val is not None:
                arr = getattr(self.input_data, key)
                if self.cb[val["cbmin"]] is not None:
                    self.fig.data[i].zmin = self.cb[val["cbmin"]]
                else:
                    self.fig.data[i].zmin = np.amin(arr[np.where(np.isfinite(arr))])
                if self.cb[val["cbmax"]] is not None:
                    self.fig.data[i].zmax = self.cb[val["cbmax"]]
                else:
                    self.fig.data[i].zmax = np.amax(arr[np.where(np.isfinite(arr))])

        # Call update_slice once to make the initial image
        self.update_axes()
        self.update_slice2d(None)
        self.vbox = [self.fig] + self.vbox
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'

        return

    def update_buttons(self, owner, event, dummy):
        toggle_slider = False
        switch_button = False
        if not self.slider[owner.dim_str].disabled:
            toggle_slider = True
            self.slider[owner.dim_str].disabled = True
        else:
            switch_button = True
        for key, button in self.buttons.items():
            if (button.value == owner.value) and (key != owner.dim_str):
                if self.slider[key].disabled:
                    button.value = owner.old_value
                else:
                    button.value = None
                button.old_value = button.value
                if toggle_slider:
                    self.slider[key].disabled = False
        owner.old_value = owner.value
        self.update_axes()
        self.update_slice2d(None)

        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        for key, button in self.buttons.items():
            if self.slider[key].disabled:
                self.fig.data[0][button.value.lower()] = self.coords[button.dim].values
                if self.show_variances:
                    func = getattr(self.fig, 'update_{}axes'.format(button.value.lower()))
                    for i in range(2):
                        func(title_text=axis_label(self.coords[button.dim]), row=1, col=i+1)
                else:
                    self.fig.update_layout({"{}axis".format(button.value.lower()) : {"title": axis_label(self.coords[button.dim])}})
        return

    # Define function to update slices
    def update_slice2d(self, change):
        # The dimensions to be sliced have been saved in slider_dims
        vslice = self.input_data
        # Slice along dimensions with active sliders
        button_dims = [None, None]
        for key, val in self.slider.items():
            if not val.disabled:
                self.lab[key].value = str(
                    self.slider_x[key][val.value])
                vslice = vslice[val.dim, val.value]
            else:
                button_dims[self.buttons[key].value.lower() == "y"] = val.dim

        # Check if dimensions of arrays agree, if not, plot the transpose
        slice_dims = vslice.dims
        transp = slice_dims == button_dims
        self.update_z2d(vslice.values, transp, self.cb["log"], 0)
        if self.show_variances:
            self.update_z2d(vslice.variances, transp, self.cb["log"], 1)
        return

    def update_z2d(self, values, transp, log, indx):
        if transp:
            values = values.T
        if log:
            with np.errstate(invalid="ignore", divide="ignore"):
                values = np.log10(values)
        self.fig.data[indx].z = values
        return


















def plot_3d(input_data, axes=None, contours=False, cb=None,
                     filename=None, name=None, config=None, figsize=None,
                     show_variances=False, ndim=0, **kwargs):
    """
    Plot a 2D slice through a N dimensional dataset. For every dimension above
    2, a slider is created to adjust the position of the slice in that
    particular dimension.
    """

    if axes is None:
        axes = input_data.dims

    if contours:
        plot_type = 'contour'
    else:
        plot_type = 'heatmap'

    # Parse colorbar
    cbar = parse_colorbar(config.cb, cb, plotly=True)

    # Make title
    title = axis_label(var=input_data, name=name, log=cbar["log"])

    if figsize is None:
        figsize = [config.width, config.height]

    layout = {"height": figsize[1], "width": figsize[0]}
    if input_data.variances is not None and show_variances:
        layout["height"] = 0.7 * layout["height"]

    # data = dict(x=[0.0],
    #             y=[0.0],
    #             z=[0.0],
    #             type=plot_type,
    #             colorscale=cbar["name"],
    #             colorbar=dict(
    #                 title=title,
    #                 titleside='right',
    #                 lenmode='fraction',
    #                 len=1.05,
    #                 thicknessmode='fraction',
    #                 thickness=0.03)
    #             )

    if ndim == 2:
        # data=[go.Surface(x=xc, y=yc, z=input_data.values, opacity=0.9, colorscale=cbar["name"])]
        data=[go.Surface(opacity=0.9, colorscale=cbar["name"])]
        fig = go.Figure(data=data, layout=layout)
        if filename is not None:
            write_image(fig=fig, file=filename)
        else:
            display(fig)
        # return
    else:
        # x1, y1 = np.meshgrid(xc, yc)
        # x2, z2 = np.meshgrid(xc, zc)
        # y3, z3 = np.meshgrid(yc, zc)
        if cbar["min"] is not None:
            zmin = cbar["min"]
        else:
            zmin = np.amin(input_data.values[np.where(np.isfinite(input_data.values))])
        if cbar["max"] is not None:
            zmax = cbar["max"]
        else:
            zmax = np.amax(input_data.values[np.where(np.isfinite(input_data.values))])
        # data=[go.Surface(x=np.zeros_like(y3), y=y3, z=z3, cmin=zmin, cmax=zmax),
        #       go.Surface(x=x2, y=np.zeros_like(x2), z=z2, cmin=zmin, cmax=zmax, showscale=False),
        #       go.Surface(x=x1, y=y1, z=np.zeros_like(x1), cmin=zmin, cmax=zmax, showscale=False)]
        data=[go.Surface(cmin=zmin, cmax=zmax),
              go.Surface(cmin=zmin, cmax=zmax, showscale=False),
              go.Surface(cmin=zmin, cmax=zmax, showscale=False)]
        # Create a SliceViewer object
        sv = Slicer3d(data=data, layout=layout,
                         input_data=input_data, axes=axes,
                         value_name=name, cb=cbar, show_variances=show_variances)
        if filename is not None:
            write_image(fig=sv.fig, file=filename)
        else:
            display(sv.vbox)

    # sv = Slicer3d(data=data, layout=layout, input_data=input_data, axes=axes,
    #               value_name=name, cb=cbar, show_variances=show_variances)

    # if filename is not None:
    #     write_image(fig=sv.fig, file=filename)
    # else:
    #     display(sv.vbox)
    # return
    return


class Slicer3d:

    def __init__(self, data, layout, input_data, axes,
                 value_name, cb, show_variances):

        self.input_data = input_data
        self.show_variances = ((self.input_data.variances is not None) and
                               show_variances)
        self.value_name = value_name
        self.cb = cb
        self.cube = None

        # Get the dimensions of the image to be displayed
        self.coords = self.input_data.coords
        self.shapes = dict(zip(self.input_data.dims, self.input_data.shape))

        # Size of the slider coordinate arrays
        self.slider_nx = dict()
        # Save dimensions tags for sliders, e.g. Dim.X
        self.slider_dims = []
        # Store coordinates of dimensions that will be in sliders
        self.slider_x = dict()
        for ax in axes:
            self.slider_dims.append(ax)
        self.ndim = len(self.slider_dims)
        for dim in self.slider_dims:
            key = str(dim)
            self.slider_nx[key] = self.shapes[dim]
            self.slider_x[key] = self.coords[dim].values
        self.nslices = len(self.slider_dims)

        # Initialise list for VBox container
        self.vbox = []

        # Initialise slider and label containers
        self.lab = dict()
        self.slider = dict()
        self.buttons = dict()
        # Default starting index for slider
        # indx = 0

        # Now begin loop to construct sliders
        button_values = [None] * (self.ndim - 3) + ['Z'] + ['Y'] + ['X']
        for i, dim in enumerate(self.slider_dims):
            key = str(dim)
            indx = (self.slider_nx[key] - 1) // 2
            # Add a label widget to display the value of the z coordinate
            self.lab[key] = widgets.Label(value=str(self.slider_x[key][indx]))
            # Add an IntSlider to slide along the z dimension of the array
            self.slider[key] = widgets.IntSlider(
                value=indx,
                min=0,
                max=self.slider_nx[key] - 1,
                step=1,
                description=key,
                continuous_update=True,
                readout=False, disabled=False #(i<self.ndim-3)
            )
            self.buttons[key] = widgets.ToggleButtons(
                options=['X', 'Y', 'Z'], description='',
                value=button_values[i],
                disabled=False,
                button_style='')
            setattr(self.buttons[key], "dim_str", key)
            setattr(self.buttons[key], "dim", dim)
            setattr(self.buttons[key], "old_value", self.buttons[key].value)
            setattr(self.slider[key], "dim_str", key)
            setattr(self.slider[key], "dim", dim)
            self.buttons[key].on_msg(self.update_buttons)
            # Add an observer to the slider
            self.slider[key].observe(self.update_slice3d, names="value")
            # Add coordinate name and unit
            self.vbox.append(widgets.HBox([self.slider[key], self.lab[key], self.buttons[key]]))

        # Initialise Figure and VBox objects
        self.fig = None
        params = {"values": {"cbmin": "min", "cbmax": "max"},
                  "variances": None}
        if self.show_variances:
            params["variances"] = {"cbmin": "min_var", "cbmax": "max_var"}

        # Set colorbar limits once to keep them constant for slicer
        # TODO: should there be auto scaling as slider value is changed?
        for i, (key, val) in enumerate(sorted(params.items())):
            if val is not None:
                arr = getattr(self.input_data, key)
                if self.cb[val["cbmin"]] is not None:
                    val["cmin"] = self.cb[val["cbmin"]]
                else:
                    val["cmin"] = np.amin(arr[np.where(np.isfinite(arr))])
                if self.cb[val["cbmax"]] is not None:
                    val["cmax"] = self.cb[val["cbmax"]]
                else:
                    val["cmax"] = np.amax(arr[np.where(np.isfinite(arr))])
        # print(params)





        # data=[go.Surface(cmin=zmin, cmax=zmax),
        #       go.Surface(cmin=zmin, cmax=zmax, showscale=False),
        #       go.Surface(cmin=zmin, cmax=zmax, showscale=False)]

        if self.show_variances:
            # params["variances"] = {"cbmin": "min_var", "cbmax": "max_var"}
            self.fig = go.FigureWidget(make_subplots(rows=1, cols=2, horizontal_spacing=0.16))
            # data["colorbar"]["x"] = 0.42
            # data["colorbar"]["thickness"] = 0.02
            for i, (key, val) in enumerate(sorted(params.items())):
                for j in range(3):
                    self.fig.add_trace(go.Surface(cmin=val["cmin"], cmax=val["cmax"], showscale=j<1), row=1, col=i+1)
            # data["colorbar"]["title"] = "variances"
            # data["colorbar"]["x"] = 1.0
            # self.fig.add_trace(data, row=1, col=2)
            # for i in range(3):
            #     self.fig.add_trace(go.Surface(cmin=zmin, cmax=zmax, showscale=i<1), row=1, col=2)
            self.fig.update_layout(height=layout["height"], width=layout["width"])
        else:
            data = [ go.Surface(cmin=params["values"]["cmin"],
                                cmax=params["values"]["cmax"],
                                showscale=j<1)
                     for j in range(3) ]
            self.fig = go.FigureWidget(data=data, layout=layout)

        # # Set colorbar limits once to keep them constant for slicer
        # # TODO: should there be auto scaling as slider value is changed?
        # for i, (key, val) in enumerate(sorted(params.items())):
        #     if val is not None:
        #         arr = getattr(self.input_data, key)
        #         if self.cb[val["cbmin"]] is not None:
        #             self.fig.data[i].zmin = self.cb[val["cbmin"]]
        #         else:
        #             self.fig.data[i].zmin = np.amin(arr[np.where(np.isfinite(arr))])
        #         if self.cb[val["cbmax"]] is not None:
        #             self.fig.data[i].zmax = self.cb[val["cbmax"]]
        #         else:
        #             self.fig.data[i].zmax = np.amax(arr[np.where(np.isfinite(arr))])

        # Call update_slice once to make the initial image
        self.update_axes()
        # self.update_slice3d(None)
        self.vbox = [self.fig] + self.vbox
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'

        return

    def update_buttons(self, owner, event, dummy):
        toggle_slider = False
        # switch_button = False
        # if not self.slider[owner.dim_str].disabled:
        #     toggle_slider = True
        #     self.slider[owner.dim_str].disabled = True
        # else:
        #     switch_button = True
        for key, button in self.buttons.items():
            if (button.value == owner.value) and (key != owner.dim_str):
                # if self.slider[key].disabled:
                #     button.value = owner.old_value
                # else:
                #     button.value = None
                button.value = owner.old_value
                button.old_value = button.value
                # if toggle_slider:
                #     self.slider[key].disabled = False
        owner.old_value = owner.value
        self.update_axes()
        # self.update_slice3d(None)

        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        titles = dict()
        buttons_dims = {"x": None, "y": None, "z": None}
        for key, button in self.buttons.items():
            # if self.slider[key].disabled:
            if button.value is not None:
                # self.fig.data[0][button.value.lower()] = self.coords[button.dim].values
                # # if self.show_variances:
                # #     func = getattr(self.fig, 'update_{}axes'.format(button.value.lower()))
                # #     for i in range(2):
                # #         func(title_text=axis_label(self.coords[button.dim]), row=1, col=i+1)
                # # else:
                # #     self.fig.update_layout({"{}axis".format(button.value.lower()) : {"title": axis_label(self.coords[button.dim])}})
                titles[button.value.lower()] = axis_label(self.coords[button.dim])
                buttons_dims[button.value.lower()] = button.dim_str

        # if self.show_variances:
        # else:
        self.fig.update_layout(scene = dict(
                    xaxis_title=titles["x"],
                    yaxis_title=titles["y"],
                    zaxis_title=titles["z"]))


        # y_x, z_x = np.meshgrid(self.slider_x[buttons_dims["y"]], self.slider_x[buttons_dims["z"]])
        # # print(y_x)
        # # print(z_x)
        # self.fig.data[0].x = np.ones_like(y_x) * self.slider[buttons_dims["x"]].value
        # self.fig.data[0].y = y_x
        # self.fig.data[0].z = z_x

        # x_y, z_y = np.meshgrid(self.slider_x[buttons_dims["x"]], self.slider_x[buttons_dims["z"]])
        # self.fig.data[1].x = x_y
        # self.fig.data[1].y = np.ones_like(x_y) * self.slider[buttons_dims["y"]].value
        # self.fig.data[1].z = z_y

        # x_z, y_z = np.meshgrid(self.slider_x[buttons_dims["x"]], self.slider_x[buttons_dims["y"]])

        # # print(self.slider_x[buttons_dims["x"]])
        # # print(buttons_dims["x"])

        # # setattr(self.fig.data[0], "x", x_z)
        # # setattr(self.fig.data[0], "y", y_z)
        # # setattr(self.fig.data[0], "z", np.ones_like(x_z) * self.slider[buttons_dims["z"]].value)
        # self.fig.data[2].x = x_z
        # self.fig.data[2].y = y_z
        # self.fig.data[2].z = np.ones_like(x_z) * self.slider[buttons_dims["z"]].value


        # # self.slider_x[idim][change["new"]] * np.ones_like(getattr(self.parent.fig.data[self.idim], self.moving_coord[self.idim])))

        # # self.fig.data[0].x = self.slider_x[idim][self.slider[idim].value] * np.ones_like(self.fig.data[0].x)
        # # # print(self.slider_x[idim][self.slider[idim].value] * np.ones_like(self.fig.data[0].x))
        # # self.fig.data[0].surfacecolor = z

        self.update_cube()

        return

    def update_cube(self, update_coordinates=True):
        # The dimensions to be sliced have been saved in slider_dims
        self.cube = self.input_data
        # Slice along dimensions with active sliders
        # button_dims = [None, None]
        for key, val in self.slider.items():
            if self.buttons[key].value is None:
                self.lab[key].value = str(
                    self.slider_x[key][val.value])
                self.cube = self.cube[val.dim, val.value]

        # The dimensions to be sliced have been saved in slider_dims
        # vslice = self.input_data
        # Slice along dimensions with active sliders
        # button_dims = [None, None]
        button_dims = dict()
        vslices = dict()
        for key, val in self.slider.items():
            if self.buttons[key].value is not None:
                loc = self.slider_x[key][val.value]
                self.lab[key].value = str(loc)
                vslices[self.buttons[key].value.lower()] = {"slice": self.cube[val.dim, val.value], "loc": loc }
                button_dims[self.buttons[key].value.lower()] = key
            # else:
            #     button_dims[self.buttons[key].value.lower() == "y"] = val.dim

        # Now make 3 slices
        permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}

        for i, (key, val) in enumerate(sorted(vslices.items())):
            if update_coordinates:
                xx, yy = np.meshgrid(self.slider_x[button_dims[permutations[key][0]]],
                                     self.slider_x[button_dims[permutations[key][1]]])
                setattr(self.fig.data[i], key, np.ones_like(xx) * val["loc"])
                setattr(self.fig.data[i], permutations[key][0], xx)
                setattr(self.fig.data[i], permutations[key][1], yy)

            # button_values = [ self.buttons[str(dim)].value.lower() for dim in val["slice"].dims ]
            # # # print(vslice.dims)
            # # print(button_values)
            # values = val["slice"].values
            # if ord(button_values[0]) < ord(button_values[1]):
            #     values = values.T
            self.fig.data[i].surfacecolor = self.check_transpose(val["slice"])


        return

    # Define function to update slices
    def update_slice3d(self, change):
        # print(change)
        # print(change["owner"].dim)
        # return
        # TODO: Possible optimization: save a 3d cube of the data for the
        # current slices with non-None button values.
        # If the slider owner has no value, then we need to re-slice everything
        # but if it does have a value (x, y, z), then we can just slice one
        # dim in the cube.
        # This possibly has a large memory overhead, duplicating an entire 3D
        # data cube.
        if self.buttons[change["owner"].dim_str].value is None:
            self.update_cube(update_coordinates=False)
        else:
            # Update only one slice

            # # The dimensions to be sliced have been saved in slider_dims
            # # vslice = self.input_data
            # # Slice along dimensions with active sliders
            # # button_dims = [None, None]
            # button_dims = dict()
            # vslices = dict()
            # for key, val in self.slider.items():
            #     if self.buttons[key].value is not None:
            key = change["owner"].dim_str

            loc = self.slider_x[key][change["new"]]
            self.lab[key].value = str(loc)
            # vslices[self.buttons[key].value.lower()] = {"slice": self.cube[change["owner"].dim, change["new"]], "dim_str": key}
            vslice = self.cube[change["owner"].dim, change["new"]]
            # button_dims[self.buttons[key].value.lower()] = key
                # else:
                #     button_dims[self.buttons[key].value.lower() == "y"] = val.dim

            # Now move slice
            # permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}
            slice_indices = {"x": 0, "y": 1, "z": 2}

            # for i, (key, val) in enumerate(sorted(vslices.items())):
            # xx, yy = np.meshgrid(self.slider_x[button_dims[permutations[key][0]]],
                                 # self.slider_x[button_dims[permutations[key][1]]])
            # print(vslice.dims)
            # print(change["owner"].dim_str)

            # # Check if dimensions of arrays agree, if not, plot the transpose
            # # slice_dims = vslice.dims
            # button_values = [ self.buttons[str(dim)].value.lower() for dim in vslice.dims ]
            # # print(vslice.dims)
            # print(button_values)
            # values = vslice.values
            # if ord(button_values[0]) < ord(button_values[1]):
            #     values = values.T
            # values = self.check_transpose(vslice)

            ax_dim = self.buttons[key].value.lower()
            xy = getattr(self.fig.data[slice_indices[ax_dim]], ax_dim)
            setattr(self.fig.data[slice_indices[ax_dim]], ax_dim, xy / xy * loc)
            # setattr(self.fig.data[i], permutations[key][0], xx)
            # setattr(self.fig.data[i], permutations[key][1], yy)
            self.fig.data[slice_indices[ax_dim]].surfacecolor = self.check_transpose(vslice)


        # # Check if dimensions of arrays agree, if not, plot the transpose
        # slice_dims = vslice.dims
        # transp = (slice_dims == button_dims)
        # self.update_z3d(vslice.values, transp, self.cb["log"], 0)
        # if self.show_variances:
        #     self.update_z3d(vslice.variances, transp, self.cb["log"], 1)
        return

    def check_transpose(self, vslice):
        # Check if dimensions of arrays agree, if not, plot the transpose
        button_values = [ self.buttons[str(dim)].value.lower() for dim in vslice.dims ]
        values = vslice.values
        if ord(button_values[0]) < ord(button_values[1]):
            values = values.T
        return values


    # def update_z3d(self, values, transp, log, indx):
    #     if transp:
    #         values = values.T
    #     if log:
    #         with np.errstate(invalid="ignore", divide="ignore"):
    #             values = np.log10(values)
    #     self.fig.data[indx].z = values
    #     return


def get_plotly_color(index=0):
    return DEFAULT_PLOTLY_COLORS[index % len(DEFAULT_PLOTLY_COLORS)]
