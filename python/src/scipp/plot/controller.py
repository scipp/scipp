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
                 logz=False,
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
        self.panel = None

        self.axes = axes
        self.logx = logx
        self.logy = logy
        self.logz = logz
        # self.slice_label = None
        self.axparams = {}
        self.profile_axparams = {}

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
        self.profile_dim = None
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
        self.labels = {}
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
            self.labels[name] = {}
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

    def _ipython_display_(self):
        return self._to_widget()._ipython_display_()

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

        self.labels[name][dim] = name_with_unit(var=var)
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

    def toggle_profile_view(self, owner=None):
        if owner is None:
            visible = False
            for but in self.widgets.profile_button.values():
                but.button_style = ""
        else:
            self.profile_dim = owner.dim
            self.profile_axparams.clear()
            if owner.button_style == "info":
                owner.button_style = ""
                visible = False
            else:
                owner.button_style = "info"
                for dim, but in self.widgets.profile_button.items():
                    if dim != self.profile_dim:
                        but.button_style = ""
                visible = True


            if visible:
                xmin = np.Inf
                xmax = np.NINF
                for name in self.xlims:
                    xlims = self.xlims[name][self.profile_dim].values
                    xmin = min(xmin, xlims[0])
                    xmax = max(xmax, xlims[1])
                self.profile_axparams = {"x": {
                    "lims": [xmin, xmax],
                    "log": False,
                    "hist": {name: self.histograms[name][self.profile_dim][self.profile_dim] for name in self.histograms},
                    "dim": self.profile_dim,
                    "label": self.labels[self.name][self.profile_dim]
                }}
                # Safety check for log axes
                if self.profile_axparams["x"]["log"] and (self.profile_axparams["x"]["lims"][0] <= 0):
                    self.profile_axparams["x"]["lims"][
                        0] = 1.0e-03 * self.profile_axparams["x"]["lims"][1]
                self.profile.update_axes(axparams=self.profile_axparams,
                                  axformatter=self.axformatter[self.name],
                                  axlocator=self.axlocator[self.name],
                                  logx=False,
                                  logy=False)
            else:
                self.view.reset_profile()


        # limits[dim] = {"button": but_val,
        # "xlims": self.xlims[self.name][dim].values,
        # "log": getattr(self, "log{}".format(but_val)),

        self.profile.toggle_view(visible=visible)
        self.toggle_hover_visibility(False)
        self.view.update_profile_connection(visible=visible)

        if visible:
            # Try to guess the y limits in a non-expensive way
            vmin, vmax = self.model.rescale_to_data()
            print(vmin, vmax)
            thickness = self.widgets.thickness_slider[self.profile_dim].value
            deltay = (vmax - vmin) / (0.25 * thickness)
            midpoint = 0.5 * (vmin + vmax)
            vmin = midpoint - deltay
            vmax = midpoint + deltay
            print(vmin, vmax, midpoint)
            self.profile.rescale_to_data(ylim=[vmin, vmax])



        # if change["new"]:
        #     self.show_profile_view()
        # else:
        #     self.hide_profile_view()
        return


    def rescale_to_data(self, button=None):

        vmin, vmax = self.model.rescale_to_data()
        self.view.rescale_to_data(vmin, vmax)
        if self.panel is not None:
            self.panel.rescale_to_data(vmin=vmin, vmax=vmax, mask_info=self.get_mask_info())



    def update_axes(self, change=None):
        self.axparams.clear()
        for dim, button in self.widgets.buttons.items():
            if self.widgets.slider[dim].disabled:
                but_val = button.value.lower()
                xmin = np.Inf
                xmax = np.NINF
                for name in self.xlims:
                    xlims = self.xlims[name][dim].values
                    xmin = min(xmin, xlims[0])
                    xmax = max(xmax, xlims[1])
                self.axparams[but_val] = {
                    "lims": [xmin, xmax],
                    "log": getattr(self, "log{}".format(but_val)),
                    "hist": {name: self.histograms[name][dim][dim] for name in self.histograms},
                    "dim": dim,
                    "label": self.labels[self.name][dim]
                }
                # Safety check for log axes
                if self.axparams[but_val]["log"] and (self.axparams[but_val]["lims"][0] <= 0):
                    self.axparams[but_val]["lims"][
                        0] = 1.0e-03 * self.axparams[but_val]["lims"][1]

                # limits[dim] = {"button": but_val,
                # "xlims": self.xlims[self.name][dim].values,
                # "log": getattr(self, "log{}".format(but_val)),
                # "hist": {name: self.histograms[name][dim][dim] for name in self.histograms}}

        self.model.update_axes(self.axparams)
        self.view.update_axes(axparams=self.axparams,
                              axformatter=self.axformatter[self.name],
                              axlocator=self.axlocator[self.name],
                              logx=self.logx,
                              logy=self.logy)
        if self.panel is not None:
            self.panel.update_axes(axparams=self.axparams)
        if self.profile is not None:
            self.toggle_profile_view()
            # self.profile.update_axes(axparams=self.axparams,
            #                   axformatter=self.axformatter[self.name],
            #                   axlocator=self.axlocator[self.name],
            #                   logx=self.logx,
            #                   logy=self.logy)
        self.update_data()
        self.rescale_to_data()


    def update_data(self, change=None):
        slices = {}
        info = {"slice_label": ""}
        # Slice along dimensions with active sliders
        for dim, val in self.widgets.slider.items():
            if not val.disabled:
                slices[dim] = {"location": val.value,
                "thickness": self.widgets.thickness_slider[dim].value}
                info["slice_label"] = "{},{}:{}-{}".format(info["slice_label"], dim,
                    slices[dim]["location"] - 0.5*slices[dim]["thickness"],
                    slices[dim]["location"] + 0.5*slices[dim]["thickness"])
        # return slices
        new_values = self.model.update_data(
            slices, mask_info=self.get_mask_info())
        self.view.update_data(new_values)
        if self.panel is not None:
            self.panel.update_data(info)

    # def update_viewport(self, xylims):
    #     new_values = self.model.update_viewport(
    #         xylims, mask_info=self.get_mask_info())
    #     self.view.update_data(new_values)

    def toggle_mask(self, change):
        self.view.toggle_mask(change)
        # if self.panel is not None:
        #     self.panel.rescale_to_data(mask_info=self.get_mask_info())

    def get_mask_info(self):
        mask_info = {}
        for name in self.widgets.mask_checkboxes:
            mask_info[name] = {m: chbx.value for m, chbx in self.widgets.mask_checkboxes[self.name].items()}
        return mask_info

    def keep_line(self, view=None, name=None, color=None, line_id=None):
        if name is None:
            name = self.name
        if view == "profile":
            self.profile.keep_line(name=name, color=color, line_id=line_id)
        else:
            self.view.keep_line(name=name, color=color, line_id=line_id)

    def remove_line(self, view=None, line_id=None):
        if view == "profile":
            self.profile.remove_line(line_id=line_id)
        else:
            self.view.remove_line(line_id=line_id)

    def update_line_color(self, line_id=None, color=None):
        self.view.update_line_color(line_id=line_id, color=color)


    def update_profile(self, xdata, ydata):
        # os.write(1, "controller: update_profile 1\n".encode())
        slices = {}
        # info = {"slice_label": ""}
        # Slice along dimensions with active sliders
        for dim, val in self.widgets.slider.items():
            if dim != self.profile_dim:
                slices[dim] = {"location": val.value,
                "thickness": self.widgets.thickness_slider[dim].value}
                # info["slice_label"] = "{},{}:{}-{}".format(info["slice_label"], dim,
                #     slices[dim]["location"] - 0.5*slices[dim]["thickness"],
                #     slices[dim]["location"] + 0.5*slices[dim]["thickness"])
        # os.write(1, "controller: update_profile 2\n".encode())
        # os.write(1, str(slices).encode())
        new_values = self.model.update_profile(xdata, ydata, slices, self.profile_axparams)
        # os.write(1, "controller: update_profile 3\n".encode())
        self.profile.update_data(new_values)
        # os.write(1, "controller: update_profile 4\n".encode())




    def toggle_hover_visibility(self, value):
        self.profile.toggle_hover_visibility(value)
        # # If the mouse moves off the image, we hide the profile. If it moves
        # # back onto the image, we show the profile
        # self.profile_viewer[self.profile_key].members["lines"][
        #     self.name].set_visible(value)
        # if self.profile_viewer[self.profile_key].errorbars[self.name]:
        #     for item in self.profile_viewer[
        #             self.profile_key].members["error_y"][self.name]:
        #         if item is not None:
        #             for it in item:
        #                 it.set_visible(value)
        # mask_dict = self.profile_viewer[self.profile_key].members["masks"][
        #     self.name]
        # if len(mask_dict) > 0:
        #     for m in mask_dict:
        #         mask_dict[m].set_visible(value if self.profile_viewer[
        #             self.profile_key].masks[self.name][m].value else False)
        #         mask_dict[m].set_gid("onaxes" if value else "offaxes")

