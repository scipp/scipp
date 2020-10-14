# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .tools import to_bin_edges, to_bin_centers, make_fake_coord
from .._utils import value_to_string
from .._scipp import core as sc
import numpy as np


class PlotModel:
    def __init__(self,
                 scipp_obj_dict=None,
                 name=None,
                 axes=None,
                 dim_to_shape=None,
                 dim_label_map=None):

        # The main container of DataArrays
        self.data_arrays = {}

        self.axformatter = {}

        axes_dims = list(axes.values())

        # Create dict of DataArrays using information from controller
        for name, array in scipp_obj_dict.items():

            # Store axis tick formatters
            self.axformatter[name] = {}

            # Create a DataArray with units of counts, and bin-edge
            # coordinates, because it is to be passed to rebin during the
            # resampling stage.
            self.data_arrays[name] = sc.DataArray(
                data=sc.Variable(dims=list(dim_to_shape[name].keys()),
                                 unit=sc.units.counts,
                                 values=array.values,
                                 variances=array.variances,
                                 dtype=sc.dtype.float64))

            # Iterate through axes and collect coordinates
            for dim in axes_dims:
                coord, formatter = self._axis_coord_and_formatter(
                    dim, array, dim_to_shape[name], dim_label_map)

                self.axformatter[name][dim] = formatter

                is_histogram = None
                for i, d in enumerate(coord.dims):
                    if d == dim:
                        is_histogram = dim_to_shape[name][
                            d] == coord.shape[i] - 1

                if is_histogram:
                    self.data_arrays[name].coords[dim] = coord
                else:
                    self.data_arrays[name].coords[dim] = to_bin_edges(
                        coord, dim)

            # Include masks
            for m, msk in array.masks.items():
                mask_dims = msk.dims
                for dim in mask_dims:
                    if dim not in axes_dims:
                        mask_dims[mask_dims.index(dim)] = dim_label_map[dim]
                self.data_arrays[name].masks[m] = sc.Variable(
                    dims=mask_dims, values=msk.values, dtype=msk.dtype)

        # The main currently displayed data slice
        self.dslice = None
        # Save a copy of the name for simpler access
        self.name = name

    def _axis_coord_and_formatter(self, dim, data_array, dim_to_shape,
                                  dim_label_map):
        """
        Get dimensions from requested axis.
        Also retun axes tick formatters and locators.
        """

        # Create some default axis tick formatter, depending on whether log
        # for that axis will be True or False
        formatter = {"linear": None, "log": None, "custom_locator": False}

        coord = None

        if dim in data_array.coords:

            underlying_dim = data_array.coords[dim].dims[-1]
            tp = data_array.coords[dim].dtype

            if tp == sc.dtype.vector_3_float64:
                coord = make_fake_coord(dim,
                                        dim_to_shape[dim] + 1,
                                        unit=data_array.coords[dim].unit)
                form = lambda val, pos: "(" + ",".join([
                    value_to_string(item, precision=2)
                    for item in data_array.coords[dim].values[int(val)]
                ]) + ")" if (int(val) >= 0 and int(val) < dim_to_shape[dim]
                             ) else ""
                formatter.update({
                    "linear": form,
                    "log": form,
                    "custom_locator": True
                })

            elif tp == sc.dtype.string:
                coord = make_fake_coord(dim,
                                        dim_to_shape[dim] + 1,
                                        unit=data_array.coords[dim].unit)
                form = lambda val, pos: data_array.coords[dim].values[int(
                    val)] if (int(val) >= 0 and int(val) < dim_to_shape[dim]
                              ) else ""
                formatter.update({
                    "linear": form,
                    "log": form,
                    "custom_locator": True
                })

            elif dim != underlying_dim:
                # non-dimension coordinate
                if underlying_dim in data_array.coords:
                    coord = data_array.coords[underlying_dim]
                    coord = sc.Variable([dim],
                                        values=coord.values,
                                        variances=coord.variances,
                                        unit=coord.unit,
                                        dtype=sc.dtype.float64)
                else:
                    coord = make_fake_coord(dim, dim_to_shape[dim] + 1)
                form = lambda val, pos: value_to_string(data_array.coords[
                    dim].values[np.abs(coord.values - val).argmin()])
                formatter.update({"linear": form, "log": form})

            else:
                coord = data_array.coords[dim].astype(sc.dtype.float64)

        else:
            # dim not found in data_array.coords
            coord = make_fake_coord(dim, dim_to_shape[dim] + 1)

        return coord, formatter

    def get_axformatter(self, name, dim):
        return self.axformatter[name][dim]

    def get_coord_center_value(self, name, dim, ind):
        return to_bin_centers(
            self.data_arrays[name].coords[dim][dim, ind:ind + 2],
            dim).values[0]

    def get_data_names(self):
        return list(self.data_arrays.keys())

    def get_data_coord(self, name, dim):
        return self.data_arrays[name].coords[dim]

    def _select_bins(self, coord, dim, start, end):
        """
        Method to speed up the finding of bin ranges
        """
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

    def resample_data(self, array, rebin_edges):
        """
        Resample a DataArray according to new bin edges.
        """
        dslice = array
        # Select bins to speed up rebinning
        for dim in rebin_edges:
            this_slice = self._select_bins(array.coords[dim], dim,
                                           rebin_edges[dim][dim, 0],
                                           rebin_edges[dim][dim, -1])
            dslice = dslice[this_slice]

        # Rebin the data
        for dim, edges in rebin_edges.items():
            dslice = sc.rebin(dslice, dim, edges)

        # Divide by pixel width
        # TODO: can this loop be combined with the one above?
        for dim, edges in rebin_edges.items():
            div = edges[dim, 1:] - edges[dim, :-1]
            div.unit = sc.units.one
            dslice /= div

        return dslice

    def rescale_to_data(self):
        """
        Get the min and max values of the currently displayed slice.
        """
        vmin = None
        vmax = None
        if self.dslice is not None:
            vmin = sc.min(self.dslice.data).value
            vmax = sc.max(self.dslice.data).value
        return vmin, vmax

    def _slice_or_rebin(self, array, dim, dim_slice):
        deltax = dim_slice["thickness"]
        if deltax == 0.0:
            array = array[dim, dim_slice["index"]]
        else:
            loc = dim_slice["location"]
            array = self.resample_data(
                array,
                rebin_edges={
                    dim:
                    sc.Variable(
                        [dim],
                        values=[loc - 0.5 * deltax, loc + 0.5 * deltax],
                        unit=array.coords[dim].unit)
                })[dim, 0]
            array *= (deltax * sc.units.one)
        return array

    def slice_data(self, array, slices):
        data_slice = array
        for dim in slices:
            data_slice = self._slice_or_rebin(data_slice, dim, slices[dim])
        return data_slice
