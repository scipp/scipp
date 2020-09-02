# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .tools import parse_params, make_fake_coord, to_bin_edges
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np
import matplotlib.ticker as ticker
import ipywidgets as widgets


class Slicer:
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 button_options=None,
                 aspect=None,
                 positions=None):

        # self.scipp_obj_dict = scipp_obj_dict
        self.data_arrays = {}

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

        # List mask names for each item
        self.masks = {}
        # Size of the slider coordinate arrays
        self.dim_to_shape = {}
        # Store coordinates of dimensions that will be in sliders
        # self.slider_coord = {}
        # Store coordinate min and max limits
        self.slider_xlims = {}
        # Store ticklabels for a dimension
        self.slider_ticks = {}
        # Store labels for sliders if any
        self.slider_label = {}
        # Record which variables are histograms along which dimension
        self.histograms = {}
        # Axes tick formatters
        self.slider_axformatter = {}
        # Axes tick locators
        self.slider_axlocator = {}
        # Save if some dims contain multi-dimensional coords
        self.contains_multid_coord = {}

        for name, array in scipp_obj_dict.items():

            self.data_arrays[name] = sc.DataArray(
                data=sc.Variable(dims=array.dims,
                                 unit=array.unit,
                                 values=array.values,
                                 variances=array.variances,
                                 dtype=sc.dtype.float64))
            self.name = name

            self.params["values"][name] = parse_params(globs=globs,
                                                       variable=array.data)

            self.params["masks"][name] = parse_params(params=masks,
                                                      defaults={
                                                          "cmap": "gray",
                                                          "cbar": False
                                                      },
                                                      globs=globs)

            # Create a map from dim to shape
            # dim_to_shape = dict(zip(array.dims, array.shape))

            # Size of the slider coordinate arrays
            self.dim_to_shape[name] = dict(zip(array.dims, array.shape))
            # Store coordinates of dimensions that will be in sliders
            # self.slider_coord[name] = {}
            # Store coordinate min and max limits
            self.slider_xlims[name] = {}
            # Store ticklabels for a dimension
            self.slider_ticks[name] = {}
            # Store labels for sliders if any
            self.slider_label[name] = {}
            # Store axis tick formatters and locators
            self.slider_axformatter[name] = {}
            self.slider_axlocator[name] = {}
            # Save information on histograms
            self.histograms[name] = {}
            # Save if some dims contain multi-dimensional coords
            self.contains_multid_coord[name] = False

            # Process axes dimensions
            if axes is None:
                axes = array.dims
            # Replace positions in axes if positions set
            if positions is not None:
                axes[axes.index(
                    self.data_array.coords[positions].dims[0])] = positions

            # Protect against duplicate entries in axes
            if len(axes) != len(set(axes)):
                raise RuntimeError("Duplicate entry in axes: {}".format(axes))
            self.ndim = len(axes)

            # Iterate through axes and collect dimensions
            for ax in axes:
                dim, var, lab, formatter, locator = self.axis_label_and_ticks(
                    ax, array, name)

                # To allow for 2D coordinates, the histograms are
                # stored as dicts, with one key per dimension of the coordinate
                # self.slider_shape[name][dim] = {}
                dim_shape = None
                self.histograms[name][dim] = {}
                for i, d in enumerate(var.dims):
                    # shape = var.shape[i]
                    self.histograms[name][dim][d] = self.dim_to_shape[name][
                        d] == var.shape[i] - 1
                    if d == dim:
                        dim_shape = var.shape[i]

                if self.histograms[name][dim][dim]:
                    self.data_arrays[name].coords[dim] = var
                else:
                    self.data_arrays[name].coords[dim] = to_bin_edges(var, dim)

                # The coordinate variable
                # self.slider_coord[name][dim] = var
                # self.data_arrays[name].coords[dim] = var
                # Store labels which can be different for coord dims if non-
                # dimension coords are used.
                self.slider_label[name][dim] = {"name": str(ax), "coord": lab}
                # The limits for each dimension
                self.slider_xlims[name][dim] = np.array(
                    [sc.min(var).value, sc.max(var).value], dtype=np.float)
                if sc.is_sorted(var, dim, order='descending'):
                    self.slider_xlims[name][dim] = np.flip(
                        self.slider_xlims[name][dim]).copy()
                # The tick formatter and locator
                self.slider_axformatter[name][dim] = formatter
                self.slider_axlocator[name][dim] = locator
                # # To allow for 2D coordinates, the shapes and histograms are
                # # stored as dicts, with one key per dimension of the coordinate
                # self.slider_shape[name][dim] = {}
                # self.histograms[name][dim] = {}
                # for i, d in enumerate(self.slider_coord[name][dim].dims):
                #     self.slider_shape[name][dim][d] = self.slider_coord[name][
                #         dim].shape[i]
                #     self.histograms[name][dim][d] = dim_to_shape[
                #         d] == self.slider_shape[name][dim][d] - 1

                # Small correction if xmin == xmax
                if self.slider_xlims[name][dim][0] == self.slider_xlims[name][
                        dim][1]:
                    if self.slider_xlims[name][dim][0] == 0.0:
                        self.slider_xlims[name][dim] = [-0.5, 0.5]
                    else:
                        dx = 0.5 * abs(self.slider_xlims[name][dim][0])
                        self.slider_xlims[name][dim][0] -= dx
                        self.slider_xlims[name][dim][1] += dx
                # For xylims, if coord is not bin-edge, we make artificial
                # bin-edge. This is simpler than finding the actual index of
                # the smallest and largest values and computing a bin edge from
                # the neighbours.
                # if not self.histograms[name][dim][
                #         dim] and self.slider_shape[name][dim][dim] > 1:
                #     dx = 0.5 * (self.slider_xlims[name][dim][1] -
                #                 self.slider_xlims[name][dim][0]) / float(
                #                     self.slider_shape[name][dim][dim] - 1)
                #     self.slider_xlims[name][dim][0] -= dx
                #     self.slider_xlims[name][dim][1] += dx
                if not self.histograms[name][dim][
                        dim] and dim_shape > 1:
                    dx = 0.5 * (self.slider_xlims[name][dim][1] -
                                self.slider_xlims[name][dim][0]) / float(
                                    dim_shape - 1)
                    self.slider_xlims[name][dim][0] -= dx
                    self.slider_xlims[name][dim][1] += dx

                self.slider_xlims[name][dim] = sc.Variable(
                    [dim], values=self.slider_xlims[name][dim], unit=var.unit)

                if len(var.dims) > 1:
                    self.contains_multid_coord[name] = True

            # Include masks
            for n, msk in array.masks:
                self.data_arrays[name].masks[n] = msk


        print(self.data_arrays)

        # Initialise list for VBox container
        self.vbox = []

        # Initialise slider and label containers
        self.lab = dict()
        self.slider = dict()
        self.thickness_slider = dict()
        self.buttons = dict()
        self.showhide = dict()
        self.button_axis_to_dim = dict()
        self.continuous_update = dict()
        # Default starting index for slider
        indx = 0

        # Additional condition if positions kwarg set
        positions_dim = None
        if len(button_options) == 3 and positions is not None:
            if self.data_array.coords[
                    positions].dtype == sc.dtype.vector_3_float64:
                positions_dim = self.data_array.coords[positions].dims[-1]
            else:
                raise RuntimeError(
                    "Supplied positions coordinate does not contain vectors.")

        # Now begin loop to construct sliders
        button_values = [None] * (self.ndim - len(button_options)) + \
            button_options[::-1]
        # for i, dim in enumerate(self.slider_coord[self.name]):
        for i, dim in enumerate(self.data_arrays[self.name].coords):
            dim_str = self.slider_label[self.name][dim]["name"]
            # Determine if slider should be disabled or not:
            # In the case of 3d projection, disable sliders that are for
            # dims < 3, or sliders that contain vectors.
            disabled = False
            if positions_dim is not None:
                disabled = dim == positions_dim
            elif i >= self.ndim - len(button_options):
                disabled = True

            # Add an IntSlider to slide along the z dimension of the array
            self.slider[dim] = widgets.IntSlider(
                value=indx,
                min=0,
                # max=self.slider_shape[self.name][dim][dim] - 1 -
                # self.histograms[name][dim][dim],
                max=self.dim_to_shape[self.name][dim] - 1,
                step=1,
                description=dim_str,
                continuous_update=True,
                readout=False,
                disabled=disabled)
            labvalue = self.make_slider_label(
                self.slider_label[self.name][dim]["coord"], indx)
            self.continuous_update[dim] = widgets.Checkbox(
                value=True,
                description="Continuous update",
                indent=False,
                layout={"width": "20px"})
            widgets.jslink((self.continuous_update[dim], 'value'),
                           (self.slider[dim], 'continuous_update'))

            if self.ndim == len(button_options):
                self.slider[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'
                # This is a trick to turn the label into the coordinate name
                # because when we hide the slider, the slider description is
                # also hidden
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
            if button_values[i] is not None:
                self.button_axis_to_dim[button_values[i].lower()] = dim
            setattr(self.buttons[dim], "dim", dim)
            setattr(self.buttons[dim], "old_value", self.buttons[dim].value)
            setattr(self.slider[dim], "dim", dim)
            setattr(self.continuous_update[dim], "dim", dim)

            # Hide buttons and labels for 1d variables
            if self.ndim == 1:
                self.buttons[dim].layout.display = 'none'
                self.lab[dim].layout.display = 'none'

            # Hide buttons and inactive sliders for 3d projection
            if len(button_options) == 3:
                self.buttons[dim].layout.display = 'none'
                if self.slider[dim].disabled:
                    self.slider[dim].layout.display = 'none'
                    self.continuous_update[dim].layout.display = 'none'
                    self.lab[dim].layout.display = 'none'

            # Add observer to buttons
            self.buttons[dim].on_msg(self.update_buttons)
            # Add an observer to the slider
            self.slider[dim].observe(self.update_slice, names="value")
            # Add the row of slider + buttons
            row = [
                self.slider[dim], self.lab[dim], self.continuous_update[dim],
                self.buttons[dim]
            ]
            self.vbox.append(widgets.HBox(row))

            # Construct members object
            self.members["widgets"]["sliders"][dim_str] = self.slider[dim]
            self.members["widgets"]["togglebuttons"][dim_str] = self.buttons[
                dim]
            self.members["widgets"]["labels"][dim_str] = self.lab[dim]

        # Add controls for masks
        self.add_masks_controls()

        return

    def make_data_array_with_bin_edges(self, array):
        da_with_edges = sc.DataArray(
            data=sc.Variable(dims=array.dims,
                             unit=array.unit,
                             values=array.values,
                             variances=array.variances,
                             dtype=sc.dtype.float64))
        for dim, coord in arraycoord[self.name].items():
            if self.histograms[self.name][dim][dim]:
                da_with_edges.coords[dim] = coord
            else:
                da_with_edges.coords[dim] = to_bin_edges(coord, dim)
        if len(self.masks[self.name]) > 0:
            for m in self.masks[self.name]:
                da_with_edges.masks[m] = self.data_array.masks[m]
        return da_with_edges

    def add_masks_controls(self):
        # Add controls for masks
        masks_found = False
        for name, array in self.data_arrays.items():
            self.masks[name] = {}
            if len(array.masks) > 0:
                masks_found = True
                for key in array.masks:
                    self.masks[name][key] = widgets.Checkbox(
                        value=self.params["masks"][name]["show"],
                        description="{}:{}".format(name, key),
                        indent=False,
                        layout={"width": "initial"})
                    setattr(self.masks[name][key], "masks_group", name)
                    setattr(self.masks[name][key], "masks_name", key)
                    self.masks[name][key].observe(self.toggle_mask,
                                                  names="value")

        if masks_found:
            self.masks_box = []
            for name in self.masks:
                mask_list = []
                for cbox in self.masks[name].values():
                    mask_list.append(cbox)
                self.masks_box.append(widgets.HBox(mask_list))
            # Add a master button to control all masks in one go
            self.masks_button = widgets.ToggleButton(
                value=self.params["masks"][self.name]["show"],
                description="Hide all masks" if
                self.params["masks"][self.name]["show"] else "Show all masks",
                disabled=False,
                button_style="")
            self.masks_button.observe(self.toggle_all_masks, names="value")
            self.vbox += [self.masks_button, widgets.VBox(self.masks_box)]
            self.members["widgets"]["togglebutton"]["masks"] = \
                self.masks_button

    def make_slider_label(self, var, indx):
        if len(var.dims) > 1:
            return "slice-{}".format(indx)
        else:
            return name_with_unit(var=var,
                                  name=value_to_string(var.values[indx]))

    def mask_to_float(self, mask, var):
        return np.where(mask, var, None).astype(np.float)

    def axis_label_and_ticks(self, axis, data_array, name):
        """
        Get dimensions and label (if present) from requested axis.
        Also retun axes tick formatters and locators.
        """

        # Create some default axis tick formatter, depending on whether log
        # for that axis will be True or False
        formatter = {
            False: ticker.ScalarFormatter(),
            True: ticker.LogFormatterSciNotation()
        }
        locator = {False: ticker.AutoLocator(), True: ticker.LogLocator()}

        dim = axis
        # Convert to Dim object?
        if isinstance(dim, str):
            dim = sc.Dim(dim)

        underlying_dim = dim
        non_dimension_coord = False
        var = None

        if dim in data_array.coords:

            dim_coord_dim = data_array.coords[dim].dims[-1]
            tp = data_array.coords[dim].dtype

            if tp == sc.dtype.vector_3_float64:
                var = make_fake_coord(dim_coord_dim,
                                      self.dim_to_shape[name][dim_coord_dim],
                                      unit=data_array.coords[dim].unit)
                form = ticker.FuncFormatter(lambda val, pos: "(" + ",".join([
                    value_to_string(item, precision=2) for item in array.coords[dim].values[int(val)]
                ]) + ")" if (int(val) >= 0 and int(val) < self.dim_to_shape[name][
                    dim_coord_dim]) else "")
                formatter.update({False: form, True: form})
                locator[False] = ticker.MaxNLocator(integer=True)
                if dim != dim_coord_dim:
                    underlying_dim = dim_coord_dim

            elif tp == sc.dtype.string:
                var = make_fake_coord(dim_coord_dim,
                                      self.dim_to_shape[name][dim_coord_dim],
                                      unit=data_array.coords[dim].unit)
                form = ticker.FuncFormatter(
                    lambda val, pos: array.coords[
                        dim].values[int(val)] if (int(val) >= 0 and int(
                            val) < self.dim_to_shape[name][dim_coord_dim]) else "")
                formatter.update({False: form, True: form})
                locator[False] = ticker.MaxNLocator(integer=True)
                if dim != dim_coord_dim:
                    underlying_dim = dim_coord_dim

            elif dim != dim_coord_dim:
                # non-dimension coordinate
                non_dimension_coord = True
                if dim_coord_dim in data_array.coords:
                    var = data_array.coords[dim_coord_dim]
                else:
                    var = make_fake_coord(dim_coord_dim,
                                          self.dim_to_shape[name][dim_coord_dim])
                underlying_dim = dim_coord_dim
                form = ticker.FuncFormatter(
                    lambda val, pos: value_to_string(data_array.coords[
                        dim].values[np.abs(var.values - val).argmin()]))
                formatter.update({False: form, True: form})

            else:
                var = data_array.coords[dim]

        else:
            # dim not found in data_array.coords
            var = make_fake_coord(dim, self.dim_to_shape[name][dim])

        label = var
        if non_dimension_coord:
            label = data_array.coords[dim]

        return underlying_dim, var, label, formatter, locator

    def update_buttons(self, change):
        return

    def toggle_all_masks(self, change):
        for name in self.masks:
            for key in self.masks[name]:
                self.masks[name][key].value = change["new"]
        change["owner"].description = "Hide all masks" if change["new"] else \
            "Show all masks"
        return
