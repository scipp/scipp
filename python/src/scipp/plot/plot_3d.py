# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..tools import axis_label, parse_colorbar
from . import config
from .plot_2d import Slicer2d
from .plot_tools import render_plot

# Other imports
import numpy as np
import ipywidgets as widgets
import plotly.graph_objs as go
from plotly.subplots import make_subplots


def plot_3d(input_data, axes=None, contours=False, cb=None, filename=None,
            name=None, figsize=None, show_variances=False, ndim=0,
            backend=None, **kwargs):
    """
    Plot a 3-slice through a N dimensional dataset. For every dimension above
    3, a slider is created to adjust the position of the slice in that
    particular dimension. For other dimensions, the sliders are used to adjust
    the position of the slice in 3D space.
    """

    if axes is None:
        axes = input_data.dims

    # Parse colorbar
    cbar = parse_colorbar(config.cb, cb, plotly=True)

    # Make title
    title = axis_label(var=input_data, name=name, log=cbar["log"])

    if figsize is None:
        figsize = [config.width, config.height]

    layout = {"height": figsize[1], "width": figsize[0]}
    if input_data.variances is not None and show_variances:
        layout["height"] = 0.7 * layout["height"]

    if ndim == 2:

        data = dict(x=[0.0],
                    y=[0.0],
                    z=[0.0],
                    type="surface",
                    colorscale=cbar["name"],
                    colorbar=dict(
                        title=title,
                        titleside='right',
                        lenmode='fraction',
                        len=1.05,
                        thicknessmode='fraction',
                        thickness=0.03)
                    )

        sv = Slicer2d(data=data, layout=layout, input_data=input_data,
                      axes=axes, value_name=title, cb=cbar,
                      show_variances=show_variances, rasterize=False,
                      surface3d=True)
    else:
        sv = Slicer3d(layout=layout, input_data=input_data, axes=axes,
                      value_name=title, cb=cbar, show_variances=show_variances)

    render_plot(static_fig=sv.fig, interactive_fig=sv.vbox, backend=backend,
                filename=filename)

    # if filename is not None:
    #     if filename.endswith(".html"):
    #         write_html(fig=sv.fig, file=filename, auto_open=False)
    #     else:
    #         write_image(fig=sv.fig, file=filename)
    # else:
    #     display_figure(static_fig=sv.fig, interactive_fig=sv.vbox,
    #                    backend=backend)
    #     # display(sv.vbox)

    return


class Slicer3d:

    def __init__(self, layout, input_data, axes,
                 value_name, cb, show_variances):

        self.input_data = input_data
        self.show_variances = ((self.input_data.variances is not None) and
                               show_variances)
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

        # Now begin loop to construct sliders
        button_values = [None] * (self.ndim - 3) + ['Z'] + ['Y'] + ['X']
        for i, dim in enumerate(self.slider_dims):
            key = str(dim)
            indx = (self.slider_nx[key] - 1) // 2
            # Add a label widget to display the value of the z coordinate
            self.lab[key] = widgets.Label(value=str(self.slider_x[key][indx]))
            # Add an IntSlider to slide along the z dimension of the array
            self.slider[key] = widgets.IntSlider(value=indx, min=0,
                                                 max=self.slider_nx[key] - 1,
                                                 step=1, description=key,
                                                 continuous_update=True,
                                                 readout=False, disabled=False)
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
            self.vbox.append(widgets.HBox([self.slider[key], self.lab[key],
                                           self.buttons[key]]))

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

        colorbars = [{"x": 1.0, "title": value_name,
                      "thicknessmode": 'fraction', "thickness": 0.02}]

        if self.show_variances:
            self.fig = go.FigureWidget(
                make_subplots(rows=1, cols=2, horizontal_spacing=0.16,
                              specs=[[{"type": "scene"}, {"type": "scene"}]]))

            colorbars.append({"x": 1.0, "title": "Variances",
                              "thicknessmode": 'fraction', "thickness": 0.02})
            colorbars[0]["x"] = -0.1

            for i, (key, val) in enumerate(sorted(params.items())):
                for j in range(3):
                    self.fig.add_trace(go.Surface(cmin=val["cmin"],
                                                  cmax=val["cmax"],
                                                  showscale=j < 1,
                                                  colorscale=self.cb["name"],
                                                  colorbar=colorbars[i]),
                                       row=1, col=i+1)
            self.fig.update_layout(height=layout["height"],
                                   width=layout["width"])
        else:
            data = [go.Surface(cmin=params["values"]["cmin"],
                               cmax=params["values"]["cmax"],
                               colorscale=self.cb["name"],
                               colorbar=colorbars[0],
                               showscale=j < 1)
                    for j in range(3)]
            self.fig = go.FigureWidget(data=data, layout=layout)

        # Call update_slice once to make the initial image
        self.update_axes()
        self.vbox = [self.fig] + self.vbox
        self.vbox = widgets.VBox(self.vbox)
        self.vbox.layout.align_items = 'center'

        return

    def update_buttons(self, owner, event, dummy):
        for key, button in self.buttons.items():
            if (button.value == owner.value) and (key != owner.dim_str):
                button.value = owner.old_value
                button.old_value = button.value
        owner.old_value = owner.value
        self.update_axes()

        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        titles = dict()
        buttons_dims = {"x": None, "y": None, "z": None}
        for key, button in self.buttons.items():
            if button.value is not None:
                titles[button.value.lower()] = axis_label(
                    self.coords[button.dim])
                buttons_dims[button.value.lower()] = button.dim_str

        axes_dict = dict(xaxis_title=titles["x"],
                         yaxis_title=titles["y"],
                         zaxis_title=titles["z"])

        if self.show_variances:
            self.fig.layout.scene1 = axes_dict
            self.fig.layout.scene2 = axes_dict
        else:
            self.fig.update_layout(scene=axes_dict)

        self.update_cube()

        return

    def update_cube(self, update_coordinates=True):
        # The dimensions to be sliced have been saved in slider_dims
        self.cube = self.input_data
        # Slice along dimensions with active sliders
        for key, val in self.slider.items():
            if self.buttons[key].value is None:
                self.lab[key].value = str(
                    self.slider_x[key][val.value])
                self.cube = self.cube[val.dim, val.value]

        # The dimensions to be sliced have been saved in slider_dims
        button_dims = dict()
        vslices = dict()
        for key, val in self.slider.items():
            if self.buttons[key].value is not None:
                loc = self.slider_x[key][val.value]
                self.lab[key].value = str(loc)
                vslices[self.buttons[key].value.lower()] = \
                    {"slice": self.cube[val.dim, val.value], "loc": loc}
                button_dims[self.buttons[key].value.lower()] = key

        # Now make 3 slices
        permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}

        for i, (key, val) in enumerate(sorted(vslices.items())):
            if update_coordinates:
                xx, yy = np.meshgrid(
                    self.slider_x[button_dims[permutations[key][0]]],
                    self.slider_x[button_dims[permutations[key][1]]])
                for j in range(1 + self.show_variances):
                    k = i + 3 * j
                    setattr(self.fig.data[k], key,
                            np.ones_like(xx) * val["loc"])
                    setattr(self.fig.data[k], permutations[key][0], xx)
                    setattr(self.fig.data[k], permutations[key][1], yy)

            self.fig.data[i].surfacecolor = self.check_transpose(val["slice"])
            if self.show_variances:
                self.fig.data[i+3].surfacecolor = self.check_transpose(
                    val["slice"], variances=True)

        return

    # Define function to update slices
    def update_slice3d(self, change):
        if self.buttons[change["owner"].dim_str].value is None:
            self.update_cube(update_coordinates=False)
        else:
            # Update only one slice
            # The dimensions to be sliced have been saved in slider_dims
            key = change["owner"].dim_str
            loc = self.slider_x[key][change["new"]]
            self.lab[key].value = str(loc)
            vslice = self.cube[change["owner"].dim, change["new"]]

            # Now move slice
            slice_indices = {"x": 0, "y": 1, "z": 2}
            ax_dim = self.buttons[key].value.lower()
            xy = getattr(self.fig.data[slice_indices[ax_dim]], ax_dim)
            for i in range(1 + self.show_variances):
                k = slice_indices[ax_dim] + (3 * (i > 0))
                setattr(self.fig.data[k], ax_dim, xy / xy * loc)
                self.fig.data[k].surfacecolor = \
                    self.check_transpose(vslice, variances=(i > 0))
        return

    def check_transpose(self, vslice, variances=False):
        # Check if dimensions of arrays agree, if not, plot the transpose
        button_values = [self.buttons[str(dim)].value.lower() for dim in
                         vslice.dims]
        if variances:
            values = vslice.variances
        else:
            values = vslice.values
        if ord(button_values[0]) < ord(button_values[1]):
            values = values.T
        return values
