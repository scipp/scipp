# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .tools import parse_params, make_fake_coord, to_bin_edges, to_bin_centers
from .widgets import PlotWidgets
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np
import matplotlib.ticker as ticker
import matplotlib.pyplot as plt
# import ipywidgets as widgets
import os


class PlotController:
    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 logx=False,
                 logy=False,
                 button_options=None,
                 # aspect=None,
                 positions=None,
                 errorbars=None):

        # os.write(1, "Slicer 1\n".encode())

        # self.parent = parent
        # self.scipp_obj_dict = scipp_obj_dict
        # self.data_arrays = {}
        # self.widgets = None

        self.model = None
        self.profile = None
        self.view = None

        self.axes = axes
        self.logx = logx
        self.logy = logy

        # # Member container for dict output
        # self.members = dict(widgets=dict(sliders=dict(),
        #                                  togglebuttons=dict(),
        #                                  togglebutton=dict(),
        #                                  buttons=dict(),
        #                                  labels=dict()))

        # Parse parameters for values, variances and masks
        self.params = {"values": {}, "variances": {}, "masks": {}}
        globs = {
            "cmap": cmap,
            "log": log,
            "vmin": vmin,
            "vmax": vmax,
            "color": color
        }
        # os.write(1, "Slicer 2\n".encode())

        # # Save aspect ratio setting
        # self.aspect = aspect
        # if self.aspect is None:
        #     self.aspect = config.plot.aspect

        # # Variables for the profile viewer
        # self.profile_viewer = None
        # self.profile_key = None
        # self.profile_dim = None
        # self.slice_pos_rectangle = None
        # self.profile_scatter = None
        # self.profile_update_lock = False
        # self.profile_ax = None
        # self.log = log
        # # self.flatten_as = flatten_as
        # # self.da_with_edges = None
        # # self.vmin = vmin
        # # self.vmax = vmax
        # # self.ylim = None


        # Containers: need one per entry in the dict of scipp
        # objects (=DataArray)
        # os.write(1, "Slicer 3\n".encode())

        # List mask names for each item
        self.mask_names = {}
        # Size of the slider coordinate arrays
        self.dim_to_shape = {}
        # Store coordinates of dimensions that will be in sliders
        self.coords = {}
        # Store coordinate min and max limits
        self.xlims = {}
        # Store ticklabels for a dimension
        # self.slider_ticks = {}
        # Store labels for sliders if any
        # self.slider_label = {}
        # Record which variables are histograms along which dimension
        self.histograms = {}
        # Axes tick formatters
        self.axformatter = {}
        # Axes tick locators
        self.axlocator = {}
        # Save if some dims contain multi-dimensional coords
        # self.contains_multid_coord = {}
        self.errorbars = {}



        if errorbars is not None:
            if isinstance(errorbars, bool):
                self.errorbars = {name: errorbars for name in scipp_obj_dict}
            elif isinstance(errorbars, dict):
                self.errorbars = errorbars
                # for name, v in errorbars.items():
                #     if name in self.data_arrays:
                #         self.errorbars[
                #             name] = errorbars[name] and self.data_arrays[
                #                 name].variances is not None
            else:
                raise TypeError("Unsupported type for argument "
                                "'errorbars': {}".format(type(errorbars)))


        # self.units = {}

        # Get first item in dict and process dimensions.
        # Dimensions should be the same for all dict items.
        self.name = list(scipp_obj_dict.keys())[0]
        self.process_axes_dimensions(scipp_obj_dict[self.name],
            positions=positions)

        for name, array in scipp_obj_dict.items():

            # # Process axes dimensions
            # if self.axes is None:
            #     self.axes = array.dims
            # # Replace positions in axes if positions set
            # if positions is not None:
            #     self.axes[self.axes.index(
            #         array.coords[positions].dims[0])] = positions

            # # Convert to Dim objects
            # for i in range(len(self.axes)):
            #     if isinstance(self.axes[i], str):
            #         self.axes[i] = sc.Dim(self.axes[i])

            # # Protect against duplicate entries in axes
            # if len(self.axes) != len(set(self.axes)):
            #     raise RuntimeError("Duplicate entry in axes: {}".format(self.axes))
            # self.ndim = len(self.axes)
            # self.process_axes_dimensions()


            # # print(array)
            array_dims = array.dims
            for dim in self.axes:
                if dim not in array_dims:
                    underlying_dim = array.coords[dim].dims[-1]
                    array_dims[array_dims.index(underlying_dim)] = dim

            # print(adims)

            # self.data_arrays[name] = sc.DataArray(
            #     data=sc.Variable(dims=adims,
            #                      unit=sc.units.counts,
            #                      values=array.values,
            #                      variances=array.variances,
            #                      dtype=sc.dtype.float64))
            # # print("================")
            # # print(self.data_arrays[name])
            # # print("================")
            # # self.name = name

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
            # self.dim_to_shape[name] = dict(zip(self.data_arrays[name].dims, self.data_arrays[name].shape))

            self.dim_to_shape[name] = dict(zip(array_dims, array.shape))



            # for dim, coord in array.coords.items():
            #     if dim not in self.dim_to_shape[name] and len(coord.dims) > 0:
            #         print(dim, self.dim_to_shape[name], coord)
            #         self.dim_to_shape[name][dim] = self.dim_to_shape[name][coord.dims[-1]]
            # print(self.dim_to_shape[name])
            # Store coordinates of dimensions that will be in sliders
            self.coords[name] = {}
            # Store coordinate min and max limits
            self.xlims[name] = {}
            # Store ticklabels for a dimension
            # self.slider_ticks[name] = {}
            # Store labels for sliders if any
            # self.slider_label[name] = {}
            # Store axis tick formatters and locators
            self.axformatter[name] = {}
            self.axlocator[name] = {}
            # Save information on histograms
            self.histograms[name] = {}
            # # Save if some dims contain multi-dimensional coords
            # self.contains_multid_coord[name] = False

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
            for dim in self.axes:
                self.collect_dim_shapes_and_lims(name, dim, array)

            # Include masks
            # for n, msk in array.masks.items():
            #     self.data_arrays[name].masks[n] = msk
            self.mask_names[name] = list(array.masks.keys())


            # Determine whether error bars should be plotted or not
            has_variances = array.variances is not None
            if name in self.errorbars:
                self.errorbars[name] &= has_variances
            else:
                self.errorbars[name] = has_variances


        # os.write(1, "Slicer 4\n".encode())

        # print(self.data_arrays)

        self.widgets = PlotWidgets(controller=self,
                         button_options=button_options)
        return

    def _to_widget(self):
        return self.widgets._to_widget()

    def process_axes_dimensions(self, array, positions=None):

        # Process axes dimensions
        if self.axes is None:
            self.axes = array.dims
        # Replace positions in axes if positions set
        if positions is not None:
            self.axes[self.axes.index(
                array.coords[positions].dims[0])] = positions

        # Convert to Dim objects
        for i in range(len(self.axes)):
            if isinstance(self.axes[i], str):
                self.axes[i] = sc.Dim(self.axes[i])

        # Protect against duplicate entries in axes
        if len(self.axes) != len(set(self.axes)):
            raise RuntimeError("Duplicate entry in axes: {}".format(self.axes))
        self.ndim = len(self.axes)
        return


    def collect_dim_shapes_and_lims(self, name, dim, array):

        var, formatter, locator = self.axis_label_and_ticks(
            dim, array, name)

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
            # self.data_arrays[name].coords[dim] = var
            self.coords[name][dim] = var
        else:
            # self.data_arrays[name].coords[dim] = to_bin_edges(var, dim)
            self.coords[name][dim] = to_bin_edges(var, dim)

        # The limits for each dimension
        self.xlims[name][dim] = np.array(
            [sc.min(var).value, sc.max(var).value], dtype=np.float)
        if sc.is_sorted(var, dim, order='descending'):
            self.xlims[name][dim] = np.flip(
                self.xlims[name][dim]).copy()
        # The tick formatter and locator
        self.axformatter[name][dim] = formatter
        self.axlocator[name][dim] = locator

        # Small correction if xmin == xmax
        if self.xlims[name][dim][0] == self.xlims[name][
                dim][1]:
            if self.xlims[name][dim][0] == 0.0:
                self.xlims[name][dim] = [-0.5, 0.5]
            else:
                dx = 0.5 * abs(self.xlims[name][dim][0])
                self.xlims[name][dim][0] -= dx
                self.xlims[name][dim][1] += dx
        # For xylims, if coord is not bin-edge, we make artificial
        # bin-edge. This is simpler than finding the actual index of
        # the smallest and largest values and computing a bin edge from
        # the neighbours.
        if not self.histograms[name][dim][
                dim] and dim_shape > 1:
            dx = 0.5 * (self.xlims[name][dim][1] -
                        self.xlims[name][dim][0]) / float(
                            dim_shape - 1)
            self.xlims[name][dim][0] -= dx
            self.xlims[name][dim][1] += dx

        self.xlims[name][dim] = sc.Variable(
            [dim], values=self.xlims[name][dim], unit=var.unit)
        return


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





    # def select_bins(self, coord, dim, start, end):
    #     bins = coord.shape[-1]
    #     if len(coord.dims) != 1:  # TODO find combined min/max
    #         return dim, slice(0, bins - 1)
    #     # scipp treats bins as closed on left and open on right: [left, right)
    #     first = sc.sum(coord <= start, dim).value - 1
    #     last = bins - sc.sum(coord > end, dim).value
    #     if first >= last:  # TODO better handling for decreasing
    #         return dim, slice(0, bins - 1)
    #     first = max(0, first)
    #     last = min(bins - 1, last)
    #     return dim, slice(first, last + 1)


    # def resample_image(self, array, rebin_edges):
    #     dslice = array
    #     # Select bins to speed up rebinning
    #     for dim in rebin_edges:
    #         this_slice = self.select_bins(array.coords[dim], dim,
    #                                       rebin_edges[dim][dim, 0],
    #                                       rebin_edges[dim][dim, -1])
    #         dslice = dslice[this_slice]

    #     # Rebin the data
    #     for dim, edges in rebin_edges.items():
    #         # print(dim)
    #         # print(dslice)
    #         # print(edges)
    #         dslice = sc.rebin(dslice, dim, edges)

    #     # Divide by pixel width
    #     for dim, edges in rebin_edges.items():
    #         # self.image_pixel_size[key] = edges.values[1] - edges.values[0]
    #         # print("edges.values[1]", edges.values[1])
    #         # print(self.image_pixel_size[key])
    #         # dslice /= self.image_pixel_size[key]
    #         div = edges[dim, 1:] - edges[dim, :-1]
    #         div.unit = sc.units.one
    #         dslice /= div
    #     return dslice

    # def mask_to_float(self, mask, var):
    #     return np.where(mask, var, None).astype(np.float)

    def toggle_profile_view(self, change=None):
        self.profile_dim = change["owner"].dim
        if change["new"]:
            self.show_profile_view()
        else:
            self.hide_profile_view()
        return


    def rescale_to_data(self, button=None):

        vmin, vmax = self.model.rescale_to_data()
        self.view.rescale_to_data(vmin, vmax)



    def update_axes(self, change=None):
        limits = {}
        for dim, button in self.widgets.buttons.items():
            if self.widgets.slider[dim].disabled:
                but_val = button.value.lower()
                limits[dim] = {"button": but_val,
                "xlims": self.xlims[self.name][dim].values,
                "log": getattr(self, "log{}".format(but_val)),
                "hist": {name: self.histograms[name][dim][dim] for name in self.histograms}}

        axparams = self.model.update_axes(limits)
        self.view.update_axes(axparams=axparams,
                              axformatter=self.axformatter[self.name],
                              axlocator=self.axlocator[self.name],
                              logx=self.logx,
                              logy=self.logy)
        self.update_data()
        self.rescale_to_data()


    def update_data(self, change=None):
        slices = {}
        # Slice along dimensions with active sliders
        for dim, val in self.widgets.slider.items():
            if not val.disabled:
                slices[dim] = {"location": val.value,
                "thickness": self.controller.widgets.thickness_slider[dim].value}
        # return slices
        new_values = self.model.update_data(slices, self.mask_names)
        self.view.update_data(new_values)

    def update_viewport_image(self, xylims):
        self.model.update_viewport_image(xylims)

    def toggle_mask(self, change):
        self.view.toggle_mask(change)
