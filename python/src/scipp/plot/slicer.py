# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .tools import parse_params
from ..utils import name_with_unit, value_to_string
from .._scipp.core import combine_masks, Variable, Dim, dtype

# Other imports
import numpy as np
import matplotlib.ticker as ticker


class Slicer:
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 values=None,
                 variances=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 button_options=None,
                 volume=False,
                 aspect=None):

        import ipywidgets as widgets

        self.scipp_obj_dict = scipp_obj_dict

        # Member container for dict output
        self.members = dict(widgets=dict(sliders=dict(),
                                         togglebuttons=dict(),
                                         togglebutton=dict(),
                                         buttons=dict(),
                                         labels=dict()))

        # Parse parameters for values, variances and masks
        self.params = {"values": {}, "variances": {}, "masks": {}}
        globs = {
            "cmap": cmap,
            "log": log,
            "vmin": vmin,
            "vmax": vmax,
            "color": color
        }

        # Save aspect ratio setting
        self.aspect = aspect
        if self.aspect is None:
            self.aspect = config.plot.aspect

        # Containers: need one per entry in the dict of scipp
        # objects (=DataArray)

        # Shape of entry
        self.shapes = {}
        # Masks are global and are combined into a single mask
        self.masks = None
        # Size of the slider coordinate arrays
        self.slider_nx = {}
        # Store coordinates of dimensions that will be in sliders
        self.slider_x = {}
        # Store ticklabels for a dimension
        self.slider_ticks = {}
        # Store labels for sliders if any
        self.slider_labels = {}
        # Record which variables are histograms along which dimension
        self.histograms = {}

        for name, array in self.scipp_obj_dict.items():

            self.data_array = array
            self.name = name

            self.params["values"][name] = parse_params(params=values,
                                                       globs=globs,
                                                       variable=array.data)

            self.params["variances"][name] = {"show": False}
            if array.variances is not None:
                self.params["variances"][name].update(
                    parse_params(params=variances,
                                 defaults={"show": False},
                                 globs=globs,
                                 variable=Variable(dims=array.dims,
                                                   values=np.sqrt(
                                                       array.variances),
                                                   unit=array.unit)))

            self.params["masks"][name] = parse_params(params=masks,
                                                      defaults={
                                                          "cmap": "gray",
                                                          "cbar": False
                                                      },
                                                      globs=globs)
            self.params["masks"][name]["show"] = (
                self.params["masks"][name]["show"] and len(array.masks) > 0)
            if self.params["masks"][name]["show"] and self.masks is None:
                self.masks = combine_masks(array.masks, array.dims,
                                           array.shape)

            # TODO: 2D coordinates will not be supported by this
            self.shapes[name] = dict(zip(array.dims, array.shape))
            for n, c in array.coords.items():
                if n not in self.shapes[name] and len(c.shape) > 0:
                    self.shapes[name][n] = c.shape[0]

            # Size of the slider coordinate arrays
            self.slider_nx[name] = {}
            # Store coordinates of dimensions that will be in sliders
            self.slider_x[name] = {}
            # Store ticklabels for a dimension
            self.slider_ticks[name] = {}
            # Store labels for sliders if any
            self.slider_labels[name] = {}

            # Process axes dimensions
            if axes is None:
                axes = array.dims
            # Protect against duplicate entries in axes
            if len(axes) != len(set(axes)):
                raise RuntimeError("Duplicate entry in axes: {}".format(axes))
            self.ndim = len(axes)

            # Iterate through axes and collect dimensions
            for ax in axes:
                dim, var, ticks = self.axis_label_and_ticks(ax, array, name)
                self.slider_x[name][dim] = var
                self.slider_ticks[name][dim] = ticks
                self.slider_nx[name][dim] = self.shapes[name][dim]

            # Save information on histograms
            self.histograms[name] = {}
            for dim, x in self.slider_x[name].items():
                self.histograms[name][dim] = self.slider_nx[name][
                    dim] == x.shape[0] - 1

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
        for i, dim in enumerate(self.slider_x[self.name].keys()):
            # If this is a 3d projection, place slices half-way
            if len(button_options) == 3 and (not volume):
                indx = (self.slider_nx[self.name][dim] - 1) // 2
            dim_str = str(dim)
            # Add an IntSlider to slide along the z dimension of the array
            self.slider[dim] = widgets.IntSlider(
                value=indx,
                min=0,
                max=self.slider_nx[self.name][dim] - 1,
                step=1,
                description=dim_str,
                continuous_update=True,
                readout=False,
                disabled=((i >= self.ndim - len(button_options))
                          and ((len(button_options) < 3) or volume)))
            labvalue = self.make_slider_label(self.slider_x[self.name][dim],
                                              indx)
            if self.ndim == len(button_options):
                self.slider[dim].layout.display = 'none'
                labvalue = dim_str
            # Add a label widget to display the value of the z coordinate
            self.lab[dim] = widgets.Label(value=labvalue)
            # Add one set of buttons per dimension
            self.buttons[dim] = widgets.ToggleButtons(
                options=button_options,
                description='',
                value=button_values[i],
                disabled=False,
                button_style='',
                style={"button_width": "70px"})
            if button_values[i] is None:
                button_style = ""
            else:
                button_style = "success"
                self.button_axis_to_dim[button_values[i].lower()] = dim
            setattr(self.buttons[dim], "dim", dim)
            setattr(self.buttons[dim], "old_value", self.buttons[dim].value)
            setattr(self.slider[dim], "dim", dim)

            if self.ndim == 1:
                self.buttons[dim].layout.display = 'none'
                self.lab[dim].layout.display = 'none'

            if (len(button_options) == 3) and (not volume):
                self.showhide[dim] = widgets.Button(
                    description="hide",
                    disabled=(button_values[i] is None),
                    button_style=button_style,
                    layout={'width': "70px"})
                setattr(self.showhide[dim], "dim", dim)
                setattr(self.showhide[dim], "value",
                        button_values[i] is not None)
                # Add observer to show/hide buttons
                self.showhide[dim].on_click(self.update_showhide)
                self.members["widgets"]["buttons"][dim] = self.showhide[dim]

            # Add observer to buttons
            self.buttons[dim].on_msg(self.update_buttons)
            # Add an observer to the slider
            self.slider[dim].observe(self.update_slice, names="value")
            # Add the row of slider + buttons
            row = [self.slider[dim], self.lab[dim], self.buttons[dim]]
            if (len(button_options) == 3) and (not volume):
                row += [
                    widgets.HTML(value="&nbsp;&nbsp;&nbsp;&nbsp;"),
                    self.showhide[dim]
                ]
            self.vbox.append(widgets.HBox(row))

            # Construct members object
            self.members["widgets"]["sliders"][dim_str] = self.slider[dim]
            self.members["widgets"]["togglebuttons"][dim_str] = self.buttons[
                dim]
            self.members["widgets"]["labels"][dim_str] = self.lab[dim]

        if self.masks is not None:
            self.masks_button = widgets.ToggleButton(
                value=self.params["masks"][self.name]["show"],
                description="Hide masks"
                if self.params["masks"][self.name]["show"] else "Show masks",
                disabled=False,
                button_style="")
            self.masks_button.observe(self.toggle_masks, names="value")
            self.vbox += [self.masks_button]
            self.members["widgets"]["togglebutton"]["masks"] = \
                self.masks_button

        return

    def make_slider_label(self, var, indx):
        return name_with_unit(var=var, name=value_to_string(var.values[indx]))

    def mask_to_float(self, mask, var):
        return np.where(mask, var, None).astype(np.float)

    def axis_label_and_ticks(self, axis, data_array, name):
        """
        Get dimensions and label (if present) from requested axis
        """
        ticks = None
        dim = axis
        # Convert to Dim object?
        if isinstance(dim, str):
            dim = Dim(dim)
        make_fake_coord = False
        fake_unit = None
        if not data_array.coords.__contains__(dim):
            make_fake_coord = True
        else:
            tp = data_array.coords[dim].dtype
            if tp == dtype.vector_3_float64:
                make_fake_coord = True
                ticks = {
                    "formatter":
                    lambda x: "(" + ",".join(
                        [value_to_string(item, precision=2)
                         for item in x]) + ")"
                }
            elif tp == dtype.string:
                make_fake_coord = True
                ticks = {"formatter": lambda x: x}
            if make_fake_coord:
                fake_unit = data_array.coords[dim].unit
                ticks["coord"] = data_array.coords[dim]
        if make_fake_coord:
            args = {"values": np.arange(self.shapes[name][dim])}
            if fake_unit is not None:
                args["unit"] = fake_unit
            var = Variable([dim], **args)
        else:
            var = data_array.coords[dim]
        return dim, var, ticks

    def get_custom_ticks(self, ax, dim, xy="x"):
        """
        Return a list of string to be used as axis tick labels in the case of
        strings or vectors as one axis coordinate.
        """
        getticks = getattr(ax, "get_{}ticks".format(xy))
        xticks = getticks()
        if xticks[2] - xticks[1] < 1:
            ax.xaxis.set_major_locator(ticker.MultipleLocator(1.0))
            xticks = getticks()
        new_ticks = [""] * len(xticks)
        for i, x in enumerate(xticks):
            if x >= 0 and x < self.slider_nx[self.name][dim]:
                new_ticks[i] = self.slider_ticks[self.name][dim]["formatter"](
                    self.slider_ticks[self.name][dim]["coord"].values[int(x)])
        return new_ticks
