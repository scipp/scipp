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


class PlotModel:
    def __init__(self,
                 controller=None,
                 scipp_obj_dict=None):

        # self.controller = controller
        self.data_arrays = {}

        # Create dict of DataArrays using information from controller
        for name, array in scipp_obj_dict.items():

            self.data_arrays[name] = sc.DataArray(
                data=sc.Variable(dims=list(controller.dim_to_shape[name].keys()),
                                 unit=sc.units.counts,
                                 values=array.values,
                                 variances=array.variances,
                                 dtype=sc.dtype.float64))

            # Add coordinates
            for dim, coord in controller.coords[name].items():
                self.data_arrays[name].coords[dim] = coord

            # Include masks
            for n, msk in array.masks.items():
                self.data_arrays[name].masks[n] = msk


        self.dslice = None
        self.name = controller.name


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
            div = edges[dim, 1:] - edges[dim, :-1]
            div.unit = sc.units.one
            dslice /= div
        return dslice

    # def mask_to_float(self, mask, var):
    #     return np.where(mask, var, None).astype(np.float)

    def toggle_profile_view(self, change=None):
        self.profile_dim = change["owner"].dim
        if change["new"]:
            self.show_profile_view()
        else:
            self.hide_profile_view()
        return

    def rescale_to_data(self):
        return sc.min(self.dslice.data).value, sc.max(self.dslice.data).value