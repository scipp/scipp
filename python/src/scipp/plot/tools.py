# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from . import config
from .._scipp.core.units import dimensionless
from .._scipp.core import Dim


# Other imports
import numpy as np


def render_plot(static_fig=None, interactive_fig=None, backend=None,
                filename=None):
    """
    Render the plot using either file export, static png inline display or
    interactive display.
    """

    # Delay imports
    import IPython.display as disp
    from plotly.io import write_html, write_image, to_image

    if filename is not None:
        if filename.endswith(".html"):
            write_html(fig=static_fig, file=filename, auto_open=False)
        else:
            write_image(fig=static_fig, file=filename)
    else:
        if backend == "static":
            disp.display(disp.Image(to_image(static_fig, format='png')))
        elif backend == "interactive":
            disp.display(interactive_fig)
        else:
            raise RuntimeError("Unknown backend {}. Currently supported "
                               "backends are 'interactive' and "
                               "'static'".format(backend))
    return


def get_color(index=0):
    """
    Get the i-th color in the list of standard colors.
    """
    return config.color_list[index % len(config.color_list)]


def edges_to_centers(x):
    """
    Convert array edges to centers
    """
    return 0.5 * (x[1:] + x[:-1])


def centers_to_edges(x):
    """
    Convert array centers to edges
    """
    e = edges_to_centers(x)
    return np.concatenate([[2.0 * x[0] - e[0]], e, [2.0 * x[-1] - e[-1]]])


def axis_label(var=None, name=None, log=False, replace_dim=True):
    """
    Make an axis label with "Name [unit]"
    """
    label = ""
    if name is not None:
        label = name
    elif var is not None:
        label = str(var.dims[0])
        if replace_dim:
            label = label.replace("Dim.", "")

    if log:
        label = "log\u2081\u2080(" + label + ")"
    if var is not None:
        if var.unit != dimensionless:
            label += " [{}]".format(var.unit)
    return label


def parse_colorbar(default, cb, plotly=False):
    """
    Construct the colorbar using default and input values
    """
    cbar = default.copy()
    if cb is not None:
        for key, val in cb.items():
            cbar[key] = val
    # In plotly, colorbar names start with an uppercase letter
    if plotly:
        cbar["name"] = cbar["name"].capitalize()
    return cbar


def axis_to_dim_label(dataset, axis):
    """
    Get dimensions and label (if present) from requested axis
    """
    if isinstance(axis, Dim):
        dim = axis
        lab = None
        var = dataset.coords[dim]
    elif isinstance(axis, str):
        # By convention, the last dim of the labels is the inner dimension,
        # but note that for now two-dimensional labels are not supported in
        # the plotting.
        dim = dataset.labels[axis].dims[-1]
        lab = axis
        var = dataset.labels[lab]
    else:
        raise RuntimeError("Unsupported axis found in 'axes': {}. This must "
                           "be either a Scipp dimension "
                           "or a string.".format(axis))
    return dim, lab, var


def get_1d_axes(var, axes, name):
    """
    Utility to simplify getting 1d axes labels and coordinate arrays
    """
    if axes is None:
        axes = {var.dims[0]: var.dims[0]}
    elif isinstance(axes, str):
        axes = {var.dims[0]: axes}
    dim, lab, xcoord = axis_to_dim_label(var, axes[var.dims[0]])
    x = xcoord.values
    xlab = axis_label(var=xcoord, name=lab)
    y = var.values
    ylab = axis_label(var=var, name="")
    return xlab, ylab, x, y


class Slicer:

    def __init__(self, input_data, axes, value_name, cb, show_variances,
                 button_options):

        import ipywidgets as widgets

        self.input_data = input_data
        self.show_variances = ((self.input_data.variances is not None) and
                               show_variances)
        self.cb = cb
        self.value_name = value_name

        # Get the dimensions of the image to be displayed
        self.coords = self.input_data.coords
        self.labels = self.input_data.labels
        self.shapes = dict(zip(self.input_data.dims, self.input_data.shape))

        # Size of the slider coordinate arrays
        self.slider_nx = dict()
        # Save dimensions tags for sliders, e.g. Dim.X
        self.slider_dims = []
        # Store coordinates of dimensions that will be in sliders
        self.slider_x = dict()
        # Store labels for sliders if any
        self.slider_labels = dict()
        # Protect against duplicate entries in axes
        if len(axes) != len(set(axes)):
            raise RuntimeError("Duplicate entry in axes: {}".format(axes))
        # Iterate through axes and collect dimensions
        for ax in axes:
            dim, lab, var = axis_to_dim_label(self.input_data, ax)
            if (lab is not None) and (dim in axes):
                raise RuntimeError("The dimension of the labels cannot also "
                                   "be specified as another axis.")
            self.slider_dims.append(dim)
            key = str(dim)
            self.slider_labels[key] = lab
            self.slider_x[key] = var
            self.slider_nx[key] = self.shapes[dim]
        self.ndim = len(self.slider_dims)

        # Initialise list for VBox container
        self.vbox = []

        # Initialise slider and label containers
        self.lab = dict()
        self.slider = dict()
        self.buttons = dict()
        # Default starting index for slider
        indx = 0

        # Now begin loop to construct sliders
        button_values = [None] * (self.ndim - len(button_options)) + \
            button_options[::-1]
        for i, dim in enumerate(self.slider_dims):
            key = str(dim)
            # If this is a 3d projection, place slices half-way
            if len(button_options) == 3:
                indx = (self.slider_nx[key] - 1) // 2
            if self.slider_labels[key] is not None:
                descr = self.slider_labels[key]
            else:
                descr = key
            # Add an IntSlider to slide along the z dimension of the array
            self.slider[key] = widgets.IntSlider(
                value=indx,
                min=0,
                max=self.slider_nx[key] - 1,
                step=1,
                description=descr,
                continuous_update=True,
                readout=False, disabled=((i >= self.ndim-2) and
                                         len(button_options) == 2)
            )
            labvalue = str(self.slider_x[key].values[indx])
            if self.ndim == 2:
                self.slider[key].layout.display = 'none'
                labvalue = descr
            # Add a label widget to display the value of the z coordinate
            self.lab[key] = widgets.Label(value=labvalue)
            # Add one set of buttons per dimension
            self.buttons[key] = widgets.ToggleButtons(
                options=button_options, description='',
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
            self.slider[key].observe(self.update_slice, names="value")
            # Add coordinate name and unit
            self.vbox.append(widgets.HBox([self.slider[key], self.lab[key],
                                           self.buttons[key]]))
        return
