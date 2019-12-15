# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .tools import axis_to_dim_label, parse_params
from .._scipp.core.units import dimensionless
from .._scipp.core import combine_masks, Variable

# Other imports
import numpy as np


class Slicer:

    def __init__(self, input_data=None, axes=None, values=None, variances=None,
                 masks=None, cmap=None, log=None, vmin=None, vmax=None,
                 color=None, button_options=None, volume=False):

        import ipywidgets as widgets

        self.input_data = input_data
        self.members = dict(widgets=dict(sliders=dict(), togglebuttons=dict(),
                            togglebutton=dict(), buttons=dict(),
                            labels=dict()))

        # Parse parameters for values, variances and masks
        self.params = dict()
        globs = {"cmap": cmap, "log": log, "vmin": vmin, "vmax": vmax,
                 "color": color}

        if hasattr(self.input_data, "values"):
            self.params["values"] = parse_params(params=values, globs=globs,
                                                 array=self.input_data.values)

        if hasattr(self.input_data, "variances"):
            self.params["variances"] = {"show": False}
            if self.input_data.variances is not None:
                self.params["variances"].update(
                    parse_params(params=variances, defaults={"show": False},
                                 globs=globs,
                                 array=np.sqrt(self.input_data.variances)))

        self.params["masks"] = parse_params(
            params=masks, defaults={"cmap": "gray", "cbar": False},
            globs=globs)
        self.params["masks"]["show"] = (self.params["masks"]["show"] and
                                        len(self.input_data.masks) > 0)
        if self.params["masks"]["show"]:
            self.masks = combine_masks(self.input_data.masks,
                                       self.input_data.dims,
                                       self.input_data.shape)

        # Get the dimensions of the image to be displayed
        self.coords = self.input_data.coords
        for shp, dim in zip(self.input_data.shape, self.input_data.dims):
            if not self.coords.__contains__(dim):
                self.input_data.coords[dim] = Variable(
                    [dim], values=np.arange(shp))

        self.labels = self.input_data.labels
        self.shapes = dict(zip(self.input_data.dims, self.input_data.shape))

        # Size of the slider coordinate arrays
        self.slider_nx = dict()
        # Save dimensions tags for sliders, e.g. Dim.X
        self.slider_dims = dict()
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
            key = str(dim)
            self.slider_dims[key] = dim
            self.slider_labels[key] = lab
            self.slider_x[key] = var
            self.slider_nx[key] = self.shapes[dim]
        self.ndim = len(self.slider_dims)

        # Save information on histograms
        self.histograms = dict()
        if hasattr(self.input_data, "name"):
            for key, x in self.slider_x.items():
                indx = self.input_data.dims.index(self.slider_dims[key])
                self.histograms[key] = self.input_data.shape[indx] == \
                    x.shape[0] - 1
        else:
            for name, var in self.input_data:
                self.histograms[name] = dict()
                for key, x in self.slider_x.items():
                    indx = var.dims.index(self.slider_dims[key])
                    self.histograms[name][key] = var.shape[indx] == \
                        x.shape[0] - 1

        # Initialise list for VBox container
        self.vbox = []

        # Initialise slider and label containers
        self.lab = dict()
        self.slider = dict()
        self.buttons = dict()
        self.showhide = dict()
        self.button_axis_to_dim = dict()
        # Default starting index for slider
        indx = 0

        # Now begin loop to construct sliders
        button_values = [None] * (self.ndim - len(button_options)) + \
            button_options[::-1]
        for i, (key, dim) in enumerate(self.slider_dims.items()):
            # If this is a 3d projection, place slices half-way
            if len(button_options) == 3 and (not volume):
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
                readout=False,
                disabled=((i >= self.ndim-len(button_options)) and
                          ((len(button_options) < 3) or volume)))
            labvalue = self.make_slider_label(self.slider_x[key], indx)
            if self.ndim == len(button_options):
                self.slider[key].layout.display = 'none'
                labvalue = descr
            # Add a label widget to display the value of the z coordinate
            self.lab[key] = widgets.Label(value=labvalue)
            # Add one set of buttons per dimension
            self.buttons[key] = widgets.ToggleButtons(
                options=button_options, description='',
                value=button_values[i],
                disabled=False,
                button_style='',
                style={"button_width": "70px"})
            if button_values[i] is None:
                button_style = ""
            else:
                button_style = "success"
                self.button_axis_to_dim[button_values[i].lower()] = key
            setattr(self.buttons[key], "dim_str", key)
            setattr(self.buttons[key], "dim", dim)
            setattr(self.buttons[key], "old_value", self.buttons[key].value)
            setattr(self.slider[key], "dim_str", key)
            setattr(self.slider[key], "dim", dim)

            if self.ndim == 1:
                self.buttons[key].layout.display = 'none'
                self.lab[key].layout.display = 'none'

            if (len(button_options) == 3) and (not volume):
                self.showhide[key] = widgets.Button(
                    description="hide",
                    disabled=(button_values[i] is None),
                    button_style=button_style,
                    layout={'width': "70px"}
                )
                setattr(self.showhide[key], "dim_str", key)
                setattr(self.showhide[key], "value",
                        button_values[i] is not None)
                # Add observer to show/hide buttons
                self.showhide[key].on_click(self.update_showhide)
                self.members["widgets"]["buttons"][key] = self.showhide[key]

            # Add observer to buttons
            self.buttons[key].on_msg(self.update_buttons)
            # Add an observer to the slider
            self.slider[key].observe(self.update_slice, names="value")
            # Add the row of slider + buttons
            row = [self.slider[key], self.lab[key], self.buttons[key]]
            if (len(button_options) == 3) and (not volume):
                row += [widgets.HTML(value="&nbsp;&nbsp;&nbsp;&nbsp;"),
                        self.showhide[key]]
            self.vbox.append(widgets.HBox(row))

            # Construct members object
            self.members["widgets"]["sliders"][key] = self.slider[key]
            self.members["widgets"]["togglebuttons"][key] = self.buttons[key]
            self.members["widgets"]["labels"][key] = self.lab[key]

        if self.params["masks"]["show"]:
            self.masks_button = widgets.ToggleButton(
                value=self.params["masks"]["show"],
                description="Hide masks" if self.params["masks"]["show"] else
                            "Show masks",
                disabled=False, button_style="")
            self.masks_button.observe(self.toggle_masks, names="value")
            self.vbox += [self.masks_button]
            self.members["widgets"]["togglebutton"]["masks"] = \
                self.masks_button

        return

    def make_slider_label(self, var, indx):
        lab = str(var.values[indx])
        if var.unit != dimensionless:
            lab += " [{}]".format(var.unit)
        return lab

    def mask_to_float(self, mask, var):
        return np.where(mask, var, None).astype(np.float)
