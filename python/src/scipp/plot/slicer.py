# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .tools import parse_params, make_fake_coord, to_bin_edges, to_bin_centers
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np
import matplotlib.ticker as ticker
import matplotlib.pyplot as plt
import ipywidgets as widgets
import os


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

        os.write(1, "Slicer 1\n".encode())

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
        os.write(1, "Slicer 2\n".encode())

        # Save aspect ratio setting
        self.aspect = aspect
        if self.aspect is None:
            self.aspect = config.plot.aspect

        # Variables for the profile viewer
        self.profile_viewer = None
        self.profile_key = None
        self.profile_dim = None
        self.slice_pos_rectangle = None
        self.profile_scatter = None
        self.profile_update_lock = False
        self.profile_ax = None
        self.log = log
        # self.flatten_as = flatten_as
        # self.da_with_edges = None
        # self.vmin = vmin
        # self.vmax = vmax
        # self.ylim = None


        # Containers: need one per entry in the dict of scipp
        # objects (=DataArray)
        os.write(1, "Slicer 3\n".encode())

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
        # self.slider_label = {}
        # Record which variables are histograms along which dimension
        self.histograms = {}
        # Axes tick formatters
        self.slider_axformatter = {}
        # Axes tick locators
        self.slider_axlocator = {}
        # Save if some dims contain multi-dimensional coords
        self.contains_multid_coord = {}

        self.units = {}

        for name, array in scipp_obj_dict.items():

            # Process axes dimensions
            if axes is None:
                axes = array.dims
            # Replace positions in axes if positions set
            if positions is not None:
                axes[axes.index(
                    array.coords[positions].dims[0])] = positions

            # Protect against duplicate entries in axes
            if len(axes) != len(set(axes)):
                raise RuntimeError("Duplicate entry in axes: {}".format(axes))
            self.ndim = len(axes)


            print(array)
            adims = array.dims
            for dim in axes:
                if dim not in adims:
                    underlying_dim = array.coords[dim].dims[-1]
                    adims[adims.index(underlying_dim)] = dim
            print(adims)

            self.data_arrays[name] = sc.DataArray(
                data=sc.Variable(dims=adims,
                                 unit=sc.units.counts,
                                 values=array.values,
                                 variances=array.variances,
                                 dtype=sc.dtype.float64))
            # print("================")
            # print(self.data_arrays[name])
            # print("================")
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
            self.dim_to_shape[name] = dict(zip(self.data_arrays[name].dims, self.data_arrays[name].shape))
            # for dim, coord in array.coords.items():
            #     if dim not in self.dim_to_shape[name] and len(coord.dims) > 0:
            #         print(dim, self.dim_to_shape[name], coord)
            #         self.dim_to_shape[name][dim] = self.dim_to_shape[name][coord.dims[-1]]
            print(self.dim_to_shape[name])
            # Store coordinates of dimensions that will be in sliders
            # self.slider_coord[name] = {}
            # Store coordinate min and max limits
            self.slider_xlims[name] = {}
            # Store ticklabels for a dimension
            self.slider_ticks[name] = {}
            # Store labels for sliders if any
            # self.slider_label[name] = {}
            # Store axis tick formatters and locators
            self.slider_axformatter[name] = {}
            self.slider_axlocator[name] = {}
            # Save information on histograms
            self.histograms[name] = {}
            # Save if some dims contain multi-dimensional coords
            self.contains_multid_coord[name] = False

            # # Process axes dimensions
            # if axes is None:
            #     axes = array.dims
            # # Replace positions in axes if positions set
            # if positions is not None:
            #     axes[axes.index(
            #         self.data_array.coords[positions].dims[0])] = positions

            # # Protect against duplicate entries in axes
            # if len(axes) != len(set(axes)):
            #     raise RuntimeError("Duplicate entry in axes: {}".format(axes))
            # self.ndim = len(axes)

            # Iterate through axes and collect dimensions
            for ax in axes:
                # dim, var, lab, formatter, locator = self.axis_label_and_ticks(
                #     ax, array, name)

                # dim = sc.Dim(ax)
                dim = ax
                # Convert to Dim object?
                if isinstance(dim, str):
                    dim = sc.Dim(dim)

                var, formatter, locator = self.axis_label_and_ticks(
                    dim, array, name)
                print(dim, var)

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
                    print("-----------------")
                    print(d, self.histograms)
                    print("-----------------")

                if self.histograms[name][dim][dim]:
                    self.data_arrays[name].coords[dim] = var
                else:
                    self.data_arrays[name].coords[dim] = to_bin_edges(var, dim)

                # The coordinate variable
                # self.slider_coord[name][dim] = var
                # self.data_arrays[name].coords[dim] = var
                # Store labels which can be different for coord dims if non-
                # dimension coords are used.
                # self.slider_label[name][dim] = {"name": str(ax), "coord": lab}
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
            for n, msk in array.masks.items():
                self.data_arrays[name].masks[n] = msk

        os.write(1, "Slicer 4\n".encode())

        print(self.data_arrays)


















        # Initialise list for VBox container
        self.rescale_button = widgets.Button(description="Rescale")
        self.rescale_button.on_click(self.rescale_to_data)
        self.vbox = [self.rescale_button]

        # Initialise slider and label containers
        self.lab = dict()
        self.slider = dict()
        # self.slider = dict()
        self.thickness_slider = dict()
        self.buttons = dict()
        self.profile_button = dict()
        self.showhide = dict()
        self.button_axis_to_dim = dict()
        self.continuous_update = dict()
        # Default starting index for slider
        indx = 0
        os.write(1, "Slicer 5\n".encode())

        # Additional condition if positions kwarg set
        # positions_dim = None
        if positions is not None:
            if scipp_obj_dict[self.name].coords[
                    positions].dtype != sc.dtype.vector_3_float64:
                raise RuntimeError(
                    "Supplied positions coordinate does not contain vectors.")
        # if len(button_options) == 3 and positions is not None:
        #     if scipp_obj_dict[self.name].coords[
        #             positions].dtype == sc.dtype.vector_3_float64:
        #         positions_dim = self.data_array.coords[positions].dims[-1]
        #     else:
        #         raise RuntimeError(
        #             "Supplied positions coordinate does not contain vectors.")

        # Now begin loop to construct sliders
        button_values = [None] * (self.ndim - len(button_options)) + \
            button_options[::-1]
        # for i, dim in enumerate(self.slider_coord[self.name]):
        # for i, dim in enumerate(self.data_arrays[self.name].coords):
        os.write(1, "Slicer 5.1\n".encode())
        for i, dim in enumerate(axes):
            # dim_str = self.slider_label[self.name][dim]["name"]
            dim_str = str(dim)
            # Determine if slider should be disabled or not:
            # In the case of 3d projection, disable sliders that are for
            # dims < 3, or sliders that contain vectors.
            disabled = False
            if positions is not None:
                disabled = dim == positions
            elif i >= self.ndim - len(button_options):
                disabled = True
            os.write(1, "Slicer 5.2\n".encode())

            # Add an IntSlider to slide along the z dimension of the array
            dim_xlims = self.slider_xlims[self.name][dim].values
            dx = dim_xlims[1] - dim_xlims[0]
            self.slider[dim] = widgets.FloatSlider(
                value=0.5 * np.sum(dim_xlims),
                min=dim_xlims[0],
                # max=self.slider_shape[self.name][dim][dim] - 1 -
                # self.histograms[name][dim][dim],
                # max=self.dim_to_shape[self.name][dim] - 1,
                max=dim_xlims[1],
                step=0.01 * dx,
                description=dim_str,
                continuous_update=True,
                readout=True,
                disabled=disabled)
            # labvalue = self.make_slider_label(
            #     self.slider_label[self.name][dim]["coord"], indx)
            self.continuous_update[dim] = widgets.Checkbox(
                value=True,
                description="Continuous update",
                indent=False,
                layout={"width": "20px"})
            widgets.jslink((self.continuous_update[dim], 'value'),
                           (self.slider[dim], 'continuous_update'))
            os.write(1, "Slicer 5.3\n".encode())

            self.thickness_slider[dim] = widgets.FloatSlider(
                value=dx,
                min=0.01 * dx,
                max=dx,
                step=0.01 * dx,
                description="Thickness",
                continuous_update=False,
                readout=True,
                disabled=disabled,
                layout={'width': "270px"})
            os.write(1, "Slicer 5.4\n".encode())

            # labvalue = self.make_slider_label(
            #         self.data_arrays[self.name].coords[dim], indx,
            #         self.slider_axformatter[self.name][dim][False])
            labvalue = "[{}]".format(self.data_arrays[self.name].coords[dim].unit)
            if self.ndim == len(button_options):
                self.slider[dim].layout.display = 'none'
                self.continuous_update[dim].layout.display = 'none'
                # This is a trick to turn the label into the coordinate name
                # because when we hide the slider, the slider description is
                # also hidden
                labvalue = dim_str
            os.write(1, "Slicer 5.5\n".encode())

            # Add a label widget to display the value of the z coordinate
            self.lab[dim] = widgets.Label(value=labvalue)
            # Add one set of buttons per dimension
            self.buttons[dim] = widgets.ToggleButtons(
                options=button_options,
                description='',
                value=button_values[i],
                disabled=False,
                button_style='',
                style={"button_width": "50px"})
            os.write(1, "Slicer 5.6\n".encode())

            self.profile_button[dim] = widgets.ToggleButton(
                value=False,
                description="Profile",
                disabled=False,
                button_style="",
                layout={"width": "initial"})
            self.profile_button[dim].observe(self.toggle_profile_view, names="value")

            os.write(1, "Slicer 5.7\n".encode())

            if button_values[i] is not None:
                self.button_axis_to_dim[button_values[i].lower()] = dim
            setattr(self.buttons[dim], "dim", dim)
            setattr(self.buttons[dim], "old_value", self.buttons[dim].value)
            setattr(self.slider[dim], "dim", dim)
            setattr(self.continuous_update[dim], "dim", dim)
            setattr(self.thickness_slider[dim], "dim", dim)
            setattr(self.profile_button[dim], "dim", dim)
            os.write(1, "Slicer 5.8\n".encode())

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
                    self.thickness_slider[dim].layout.display = 'none'
            os.write(1, "Slicer 5.9\n".encode())

            # Add observer to buttons
            self.buttons[dim].on_msg(self.update_buttons)
            # Add an observer to the sliders
            self.slider[dim].observe(self.update_slice, names="value")
            self.thickness_slider[dim].observe(self.update_slice, names="value")
            # Add the row of slider + buttons
            row = [
                self.slider[dim], self.lab[dim], self.continuous_update[dim],
                self.buttons[dim], self.thickness_slider[dim],
                self.profile_button[dim]
            ]
            self.vbox.append(widgets.HBox(row))
            os.write(1, "Slicer 5.10\n".encode())

            # Construct members object
            self.members["widgets"]["sliders"][dim_str] = self.slider[dim]
            self.members["widgets"]["togglebuttons"][dim_str] = self.buttons[
                dim]
            self.members["widgets"]["labels"][dim_str] = self.lab[dim]
        os.write(1, "Slicer 6\n".encode())

        # Add controls for masks
        self.add_masks_controls()
        os.write(1, "Slicer 7\n".encode())

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

    # def make_slider_label(self, var, indx, formatter):
    def make_slider_label(self, value, formatter):
        # if len(var.dims) > 1:
        #     return "slice-{}".format(indx)
        # else:
        #     return name_with_unit(var=var,
        #                           name=formatter.format_data_short(var.values[indx]))
        #                           # name=value_to_string(var.values[indx]))
        return formatter.format_data_short(value)

    def mask_to_float(self, mask, var):
        return np.where(mask, var, None).astype(np.float)

    def axis_label_and_ticks(self, dim, data_array, name):
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

        # dim = axis
        # Convert to Dim object?
        # if isinstance(dim, str):
        #     dim = sc.Dim(dim)

        # underlying_dim = dim
        # non_dimension_coord = False
        var = None

        if dim in data_array.coords:

            dim_coord_dim = data_array.coords[dim].dims[-1]
            tp = data_array.coords[dim].dtype

            if tp == sc.dtype.vector_3_float64:
                # var = make_fake_coord(dim_coord_dim,
                                      # self.dim_to_shape[name][dim_coord_dim],
                var = make_fake_coord(dim,
                                      self.dim_to_shape[name][dim],
                                      unit=data_array.coords[dim].unit)
                form = ticker.FuncFormatter(lambda val, pos: "(" + ",".join([
                    value_to_string(item, precision=2) for item in data_array.coords[dim].values[int(val)]
                ]) + ")" if (int(val) >= 0 and int(val) < self.dim_to_shape[name][
                    dim]) else "")
                    # dim_coord_dim]) else "")
                formatter.update({False: form, True: form})
                locator[False] = ticker.MaxNLocator(integer=True)
                # if dim != dim_coord_dim:
                #     underlying_dim = dim_coord_dim

            elif tp == sc.dtype.string:
                # var = make_fake_coord(dim_coord_dim,
                #                       self.dim_to_shape[name][dim_coord_dim],
                var = make_fake_coord(dim,
                                      self.dim_to_shape[name][dim],
                                      unit=data_array.coords[dim].unit)
                form = ticker.FuncFormatter(
                    lambda val, pos: data_array.coords[
                        dim].values[int(val)] if (int(val) >= 0 and int(
                            val) < self.dim_to_shape[name][dim]) else "")
                formatter.update({False: form, True: form})
                locator[False] = ticker.MaxNLocator(integer=True)
                # if dim != dim_coord_dim:
                #     underlying_dim = dim_coord_dim

            elif dim != dim_coord_dim:
                # non-dimension coordinate
                # non_dimension_coord = True
                if dim_coord_dim in data_array.coords:
                    coord = data_array.coords[dim_coord_dim]
                    var = sc.Variable(
                        [dim],
                        values=coord.values,
                        variances=coord.variances,
                        unit=coord.unit,
                        dtype=sc.dtype.float64)
                else:
                    var = make_fake_coord(dim,
                                          self.dim_to_shape[name][dim_coord_dim])
                # underlying_dim = dim_coord_dim
                # var = data_array.coords[dim].astype(sc.dtype.float64)
                form = ticker.FuncFormatter(
                    lambda val, pos: value_to_string(data_array.coords[
                        dim].values[np.abs(var.values - val).argmin()]))
                formatter.update({False: form, True: form})

            else:
                var = data_array.coords[dim].astype(sc.dtype.float64)

        else:
            # dim not found in data_array.coords
            var = make_fake_coord(dim, self.dim_to_shape[name][dim])

        # label = var
        # if non_dimension_coord:
        #     label = data_array.coords[dim]

        # return underlying_dim, var, label, formatter, locator
        return var, formatter, locator

    def update_buttons(self, change):
        return

    def toggle_all_masks(self, change):
        for name in self.masks:
            for key in self.masks[name]:
                self.masks[name][key].value = change["new"]
        change["owner"].description = "Hide all masks" if change["new"] else \
            "Show all masks"
        return

    def select_bins(self, coord, dim, start, end):
        bins = coord.shape[-1]
        if len(coord.dims) != 1:  # TODO find combined min/max
            return dim, slice(0, bins - 1)
        # scipp treats bins as closed on left and open on right: [left, right)
        first = sc.sum(coord <= start, dim).value - 1
        last = bins - sc.sum(coord > end, dim).value
        if first >= last:  # TODO better handling for decreasing
            return dim, slice(0, bins - 1)
        first = max(0, first)
        last = min(bins - 1, last)
        return dim, slice(first, last + 1)


    def resample_image(self, array, rebin_edges):
        dslice = array
        # Select bins to speed up rebinning
        for dim in rebin_edges:
            this_slice = self.select_bins(array.coords[dim], dim,
                                          rebin_edges[dim][dim, 0],
                                          rebin_edges[dim][dim, -1])
            dslice = dslice[this_slice]

        # Rebin the data
        for dim, edges in rebin_edges.items():
            # print(dim)
            # print(dslice)
            # print(edges)
            dslice = sc.rebin(dslice, dim, edges)

        # Divide by pixel width
        for dim, edges in rebin_edges.items():
            # self.image_pixel_size[key] = edges.values[1] - edges.values[0]
            # print("edges.values[1]", edges.values[1])
            # print(self.image_pixel_size[key])
            # dslice /= self.image_pixel_size[key]
            dslice /= edges[dim, 1:] - edges[dim, :-1]
        return dslice





# ################################################################
# # Profile viewer ###############################################
# ################################################################



    def toggle_profile_view(self, change=None):
        return
        # self.profile_dim = change["owner"].dim
        # if change["new"]:
        #     self.show_profile_view()
        # else:
        #     self.hide_profile_view()
        # return



#     def show_profile_view(self):

#         # Double the figure height
#         self.fig.set_figheight(2 * self.fig.get_figheight())
#         # Change the ax geometry so it becomes a subplot
#         self.ax.change_geometry(2, 1, 1)
#         # Add lower panel
#         self.profile_ax = self.fig.add_subplot(212)

#         # Also need to move the colorbar to the top panel.
#         # Easiest way to do this is to remove it and create it again.
#         if self.params["values"][self.name]["cbar"]:
#             self.cbar.remove()
#             del self.cbar
#             self.cbar = plt.colorbar(self.image, ax=self.ax, cax=self.cax)
#             self.cbar.set_label(name_with_unit(var=self.data_arrays[self.name], name=""))
#             if self.cax is None:
#                 self.cbar.ax.yaxis.set_label_coords(-1.1, 0.5)
#             self.members["colorbar"] = self.cbar

#         # # self.ax_pick.set_ylim([
#         # #     self.params["values"][self.name]["vmin"],
#         # #     self.params["values"][self.name]["vmax"]
#         # # ])
#         # self.ax_pick.set_ylim(get_ylim(
#         #         var=self.data_array, errorbars=(self.data_array.variances is not None)))

#         # Connect picking events
#         # self.fig.canvas.mpl_connect('pick_event', self.keep_or_delete_profile)
#         self.fig.canvas.mpl_connect('motion_notify_event', self.update_profile)

#         return

#     def update_profile_axes(self):

#         # Clear profile axes if present and reset to None
#         del self.profile_viewer
#         if self.profile_ax is not None:
#             self.profile_ax.clear()
#             # # ylim = get_ylim(
#             # #     var=self.data_array, errorbars=(self.data_array.variances is not None))
#             # self.ax_pick.set_ylim(get_ylim(
#             #     var=self.data_array, errorbars=(self.data_array.variances is not None)))
#         self.profile_viewer = None
#         if self.profile_scatter is not None:
#             # self.ax.collections = []
#             self.fig.canvas.draw_idle()
#             del self.profile_scatter
#             self.profile_scatter = None








#     def compute_profile(self, event):
#         # Find indices of pixel where cursor lies
#         # os.write(1, "compute_profile 1\n".encode())
#         dimx = self.xyrebin["x"].dims[0]
#         # os.write(1, "compute_profile 1.1\n".encode())
#         dimy = self.xyrebin["y"].dims[0]
#         # os.write(1, "compute_profile 1.2\n".encode())
#         ix = int((event.xdata - self.current_lims["x"][0]) /
#                  (self.xyrebin["x"].values[1] - self.xyrebin["x"].values[0]))
#         # os.write(1, "compute_profile 1.3\n".encode())
#         iy = int((event.ydata - self.current_lims["y"][0]) /
#                  (self.xyrebin["y"].values[1] - self.xyrebin["y"].values[0]))
#         # os.write(1, "compute_profile 2\n".encode())

#         data_slice = self.data_arrays[self.name]
#         os.write(1, "compute_profile 3\n".encode())

#         # Slice along dimensions with active sliders
#         for dim, val in self.slider.items():
#             os.write(1, "compute_profile 4\n".encode())
#             if dim != self.profile_dim:
#                 os.write(1, "compute_profile 5\n".encode())
#                 if dim == dimx:
#                     os.write(1, "compute_profile 6\n".encode())
#                     data_slice = self.resample_image(data_slice,
#                         rebin_edges={dimx: self.xyrebin["x"][dimx, ix:ix + 2]})[dimx, 0]
#                 elif dim == dimy:
#                     os.write(1, "compute_profile 7\n".encode())
#                     data_slice = self.resample_image(data_slice,
#                         rebin_edges={dimy: self.xyrebin["y"][dimy, iy:iy + 2]})[dimy, 0]
#                 else:
#                     os.write(1, "compute_profile 8\n".encode())
#                     deltax = self.thickness_slider[dim].value
#                     data_slice = self.resample_image(data_slice,
#                         rebin_edges={dim: sc.Variable([dim], values=[val.value - 0.5 * deltax,
#                                                                      val.value + 0.5 * deltax],
#                                                             unit=data_slice.coords[dim].unit)})[dim, 0]
#         os.write(1, "compute_profile 9\n".encode())

#                     # depth = self.slider_xlims[self.name][dim][dim, 1] - self.slider_xlims[self.name][dim][dim, 0]
#                     # depth.unit = sc.units.one
#                 # data_slice *= (deltax * sc.units.one)


#         # # Resample the 3d cube down to a 1d profile
#         # return self.resample_image(self.da_with_edges,
#         #                            coord_edges={
#         #                                dimy: self.da_with_edges.coords[dimy],
#         #                                dimx: self.da_with_edges.coords[dimx]
#         #                            },
#         #                            rebin_edges={
#         #                                dimy: self.xyrebin["y"][dimy,
#         #                                                        iy:iy + 2],
#         #                                dimx: self.xyrebin["x"][dimx, ix:ix + 2]
#         #                            })[dimy, 0][dimx, 0]
#         return data_slice

#     def create_profile_plot(self, prof):
#         # We need to extract the data again and replace with the original
#         # coordinates, because coordinates have been forced to be bin-edges
#         # so that rebin could be used. Also reset original unit.
#         os.write(1, "create_profile_viewer 1\n".encode())
#         # to_plot = sc.DataArray(data=sc.Variable(dims=prof.dims,
#         #                                         unit=self.data_arrays[self.name].unit,
#         #                                         values=prof.values,
#         #                                         variances=prof.variances))

#         prof.unit = self.data_arrays[self.name].unit
#         os.write(1, "create_profile_viewer 1.1\n".encode())
#         dim = prof.dims[0]
#         os.write(1, "create_profile_viewer 1.2\n".encode())
#         if not self.histograms[self.name][dim][dim]:
#             os.write(1, "create_profile_viewer 1.3\n".encode())
#             os.write(1, (str(to_bin_centers(prof.coords[dim], dim)) + "\n").encode())
#             # TODO: there is an issue here when we have non-bin edges
#             prof.coords[dim] = to_bin_centers(prof.coords[dim], dim)

#         os.write(1, "create_profile_viewer 2\n".encode())
#         # for dim in prof.dims:
#         #     to_plot.coords[dim] = self.slider_coord[self.name][dim]
#         # if len(prof.masks) > 0:
#         #     for m in prof.masks:
#         #         to_plot.masks[m] = prof.masks[m]
#         # os.write(1, "create_profile_viewer 3\n".encode())
#         self.profile_viewer = plot({self.name: prof},
#                                    ax=self.profile_ax,
#                                    logy=self.log)
#         os.write(1, "create_profile_viewer 3\n".encode())
#         self.profile_key = list(self.profile_viewer.keys())[0]
#         os.write(1, "create_profile_viewer 4\n".encode())
#         # if self.flatten_as != "slice":
#         #     self.ax_pick.set_ylim(self.ylim)
#         # os.write(1, "create_profile_viewer 5\n".encode())
#         return prof



#     def update_profile(self, event):
#         os.write(1, "update_profile 1\n".encode())
#         if event.inaxes == self.ax:
#             os.write(1, "update_profile 1.5\n".encode())
#             prof = self.compute_profile(event)
#             os.write(1, "update_profile 2\n".encode())
#             if self.profile_viewer is None:
#                 to_plot = self.create_profile_plot(prof)
#                 os.write(1, "update_profile 3\n".encode())

#                 # if self.flatten_as == "slice":

#                 # # Add indicator of range covered by current slice
#                 # dim = to_plot.dims[0]
#                 # xlims = self.ax_pick.get_xlim()
#                 # ylims = self.ax_pick.get_ylim()
#                 # left = to_plot.coords[dim][dim, self.slider[dim].value].value
#                 # if self.histograms[self.name][dim][dim]:
#                 #     width = (
#                 #         to_plot.coords[dim][dim, self.slider[dim].value + 1] -
#                 #         to_plot.coords[dim][dim, self.slider[dim].value]).value
#                 # else:
#                 #     width = 0.01 * (xlims[1] - xlims[0])
#                 #     left -= 0.5 * width
#                 # self.slice_pos_rectangle = Rectangle((left, ylims[0]),
#                 #                                      width,
#                 #                                      ylims[1] - ylims[0],
#                 #                                      facecolor="lightgray",
#                 #                                      zorder=-10)
#                 # self.ax_pick.add_patch(self.slice_pos_rectangle)




#             # else:
#             #     # os.write(1, "update_profile 5\n".encode())
#             #     self.profile_viewer[self.profile_key].update_slice(
#             #         {"vslice": {
#             #             self.name: prof
#             #         }})



#                 # os.write(1, "update_profile 6\n".encode())
#             # os.write(1, "update_profile 7\n".encode())




#             self.toggle_visibility_of_hover_plot(True)
#         elif self.profile_viewer is not None:
#             self.toggle_visibility_of_hover_plot(False)




#         # self.fig.canvas.draw_idle()
#         # os.write(1, "update_profile 8\n".encode())

#     def toggle_visibility_of_hover_plot(self, value):
#         return
#         # If the mouse moves off the image, we hide the profile. If it moves
#         # back onto the image, we show the profile
#         self.profile_viewer[self.profile_key].members["lines"][
#             self.name].set_visible(value)
#         if self.profile_viewer[self.profile_key].errorbars[self.name]:
#             for item in self.profile_viewer[
#                     self.profile_key].members["error_y"][self.name]:
#                 if item is not None:
#                     for it in item:
#                         it.set_visible(value)
#         mask_dict = self.profile_viewer[self.profile_key].members["masks"][
#             self.name]
#         if len(mask_dict) > 0:
#             for m in mask_dict:
#                 mask_dict[m].set_visible(value if self.profile_viewer[
#                     self.profile_key].masks[self.name][m].value else False)
#                 mask_dict[m].set_gid("onaxes" if value else "offaxes")

#     def keep_or_delete_profile(self, event):
#         if isinstance(event.artist, PathCollection):
#             self.delete_profile(event)
#             self.profile_update_lock = True
#         elif self.profile_update_lock:
#             self.profile_update_lock = False
#         else:
#             self.keep_profile(event)

#     def keep_profile(self, event):
#         trace = list(
#             self.profile_viewer[self.profile_key].keep_buttons.values())[-1]
#         xdata = event.mouseevent.xdata
#         ydata = event.mouseevent.ydata
#         if self.profile_scatter is None:
#             self.profile_scatter = self.ax.scatter(
#                 [xdata], [ydata], c=[trace["colorpicker"].value], picker=5)
#         else:
#             new_offsets = np.concatenate(
#                 (self.profile_scatter.get_offsets(), [[xdata, ydata]]), axis=0)
#             col = np.array(_hex_to_rgb(trace["colorpicker"].value) + [255],
#                            dtype=np.float) / 255.0
#             new_colors = np.concatenate(
#                 (self.profile_scatter.get_facecolors(), [col]), axis=0)
#             self.profile_scatter.set_offsets(new_offsets)
#             self.profile_scatter.set_facecolors(new_colors)
#         self.profile_viewer[self.profile_key].keep_trace(trace["button"])

#     def delete_profile(self, event):
#         ind = event.ind[0]
#         xy = np.delete(self.profile_scatter.get_offsets(), ind, axis=0)
#         c = np.delete(self.profile_scatter.get_facecolors(), ind, axis=0)
#         self.profile_scatter.set_offsets(xy)
#         self.profile_scatter.set_facecolors(c)
#         self.fig.canvas.draw_idle()
#         # Also remove the line from the 1d plot
#         trace = list(
#             self.profile_viewer[self.profile_key].keep_buttons.values())[ind]
#         self.profile_viewer[self.profile_key].remove_trace(trace["button"])
