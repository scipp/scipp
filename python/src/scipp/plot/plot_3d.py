# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..config import plot as config
from .plot_2d import Slicer2d
from .render import render_plot
from .slicer import Slicer
from .tools import axis_label, parse_colorbar

# Other imports
import numpy as np
import ipywidgets as widgets
import plotly.graph_objs as go
from plotly.subplots import make_subplots


def plot_3d(input_data, axes=None, contours=False, cb=None, filename=None,
            name=None, figsize=None, show_variances=False, ndim=0,
            backend=None):
    """
    Plot a 3-slice through a N dimensional dataset. For every dimension above
    3, a slider is created to adjust the position of the slice in that
    particular dimension. For other dimensions, the sliders are used to adjust
    the position of the slice in 3D space.
    """

    var = input_data[name]
    if axes is None:
        axes = var.dims

    # Parse colorbar
    cbar = parse_colorbar(cb, plotly=True)

    # Make title
    title = axis_label(var=var, name=name, log=cbar["log"])

    if figsize is None:
        figsize = [config.width, config.height]

    layout = {"height": figsize[1], "width": figsize[0], "showlegend": False}
    if var.variances is not None and show_variances:
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

        sv = Slicer2d(data=data, layout=layout, input_data=var,
                      axes=axes, value_name=title, cb=cbar,
                      show_variances=show_variances, rasterize=False,
                      surface3d=True)
    else:
        sv = Slicer3d(layout=layout, input_data=var, axes=axes,
                      value_name=title, cb=cbar, show_variances=show_variances)

    render_plot(static_fig=sv.fig, interactive_fig=sv.vbox, backend=backend,
                filename=filename)

    return


class Slicer3d(Slicer):

    def __init__(self, layout, input_data, axes,
                 value_name, cb, show_variances):

        super().__init__(input_data, axes, value_name, cb, show_variances,
                         button_options=['X', 'Y', 'Z'])

        self.cube = None
        self.slice_indices = {"x": 0, "y": 1, "z": 2}

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

        # Store min/max for each dimension for invisible scatter
        self.xminmax = dict()
        for key, var in self.slider_x.items():
            self.xminmax[key] = [var.values[0], var.values[-1]]
        scatter_x, scatter_y, scatter_z = self.get_outline_as_scatter()

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
                                                  showscale=False,
                                                  colorscale=self.cb["name"],
                                                  colorbar=colorbars[i]),
                                       row=1, col=i+1)
                self.fig.add_trace(
                    go.Scatter3d(x=scatter_x,
                                 y=scatter_y,
                                 z=scatter_z,
                                 marker=dict(cmin=val["cmin"],
                                             cmax=val["cmax"],
                                             color=np.linspace(
                                                 val["cmin"],
                                                 val["cmax"], 8),
                                             colorbar=colorbars[i],
                                             colorscale=self.cb["name"],
                                             showscale=True,
                                             opacity=1.0e-6),
                                 mode="markers",
                                 hoverinfo="none"),
                    row=1, col=i+1)
            self.fig.update_layout(**layout)
        else:
            data = [go.Surface(cmin=params["values"]["cmin"],
                               cmax=params["values"]["cmax"],
                               colorscale=self.cb["name"],
                               colorbar=colorbars[0],
                               showscale=False)
                    for j in range(3)]

            data += [go.Scatter3d(x=scatter_x,
                                  y=scatter_y,
                                  z=scatter_z,
                                  marker=dict(cmin=params["values"]["cmin"],
                                              cmax=params["values"]["cmax"],
                                              color=np.linspace(
                                                  params["values"]["cmin"],
                                                  params["values"]["cmax"], 8),
                                              colorbar=colorbars[0],
                                              colorscale=self.cb["name"],
                                              showscale=True,
                                              opacity=1.0e-6),
                                  mode="markers",
                                  hoverinfo="none")]
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
        # Update the show/hide checkboxes
        for key, button in self.buttons.items():
            # ax_dim = self.buttons[key].value
            ax_dim = button.value
            if ax_dim is not None:
                ax_dim = ax_dim.lower()
                for j in range(1 + self.show_variances):
                    k = self.slice_indices[ax_dim] + 4 * j
                    self.fig.data[k].visible = True
            self.showhide[key].value = (button.value is not None)
            self.showhide[key].disabled = (button.value is None)
            self.showhide[key].description = "hide"
            if button.value is None:
                self.showhide[key].button_style = ""
            else:
                self.showhide[key].button_style = "success"
                self.button_axis_to_dim[ax_dim] = key
        # Update the scatter
        scatter_x, scatter_y, scatter_z = self.get_outline_as_scatter()
        self.fig.data[3].update(x=scatter_x, y=scatter_y, z=scatter_z)
        if self.show_variances:
            self.fig.data[7].update(x=scatter_x, y=scatter_y, z=scatter_z)
        self.update_axes()

        return

    def update_axes(self):
        # Go through the buttons and select the right coordinates for the axes
        titles = dict()
        buttons_dims = {"x": None, "y": None, "z": None}
        for key, button in self.buttons.items():
            if button.value is not None:
                titles[button.value.lower()] = axis_label(
                    self.slider_x[key], name=self.slider_labels[key])
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
                    self.slider_x[key].values[val.value])
                self.cube = self.cube[val.dim, val.value]

        # The dimensions to be sliced have been saved in slider_dims
        button_dims = dict()
        vslices = dict()
        for key, val in self.slider.items():
            if self.buttons[key].value is not None:
                loc = self.slider_x[key].values[val.value]
                self.lab[key].value = str(loc)
                vslices[self.buttons[key].value.lower()] = \
                    {"slice": self.cube[val.dim, val.value], "loc": loc}
                button_dims[self.buttons[key].value.lower()] = key

        # Now make 3 slices
        permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}

        for i, (key, val) in enumerate(sorted(vslices.items())):
            if update_coordinates:
                xx, yy = np.meshgrid(
                    self.slider_x[button_dims[permutations[key][0]]].values,
                    self.slider_x[button_dims[permutations[key][1]]].values)
                for j in range(1 + self.show_variances):
                    k = i + 4 * j
                    setattr(self.fig.data[k], key,
                            np.ones_like(xx) * val["loc"])
                    setattr(self.fig.data[k], permutations[key][0], xx)
                    setattr(self.fig.data[k], permutations[key][1], yy)

            self.fig.data[i].surfacecolor = self.check_transpose(val["slice"])
            if self.show_variances:
                self.fig.data[i+4].surfacecolor = self.check_transpose(
                    val["slice"], variances=True)

        return

    # Define function to update slices
    def update_slice(self, change):
        if self.buttons[change["owner"].dim_str].value is None:
            self.update_cube(update_coordinates=False)
        else:
            # Update only one slice
            # The dimensions to be sliced have been saved in slider_dims
            key = change["owner"].dim_str
            loc = self.slider_x[key].values[change["new"]]
            self.lab[key].value = str(loc)
            vslice = self.cube[change["owner"].dim, change["new"]]

            # Now move slice
            ax_dim = self.buttons[key].value.lower()
            xy = getattr(self.fig.data[self.slice_indices[ax_dim]], ax_dim)
            for i in range(1 + self.show_variances):
                k = self.slice_indices[ax_dim] + 4 * i
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

    def update_showhide(self, owner):
        owner.value = not owner.value
        owner.description = "hide" if owner.value else "show"
        owner.button_style = "success" if owner.value else "danger"
        key = owner.dim_str
        ax_dim = self.buttons[key].value.lower()
        for i in range(1 + self.show_variances):
            k = self.slice_indices[ax_dim] + 4 * i
            self.fig.data[k].visible = owner.value
        return

    def get_outline_as_scatter(self):
        scatter_x, scatter_y, scatter_z = np.meshgrid(
                self.xminmax[self.button_axis_to_dim["x"]],
                self.xminmax[self.button_axis_to_dim["y"]],
                self.xminmax[self.button_axis_to_dim["z"]])
        return scatter_x.flatten(), scatter_y.flatten(), scatter_z.flatten()
