# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .model import PlotModel
from .tools import to_bin_centers, vars_to_err, mask_to_float
from .._scipp import core as sc

# Other imports
import numpy as np


class PlotModel1d(PlotModel):
    def __init__(self, *args, **kwargs):

        super().__init__(*args, **kwargs)

        self.dim = None
        self.hist = None

        return

    def update_axes(self, axparams):
        self.dim = axparams["x"]["dim"]
        self.hist = axparams["x"]["hist"]
        return

    def update_data(self, slices, mask_info):

        new_values = {}

        for name, array in self.data_arrays.items():

            new_values[name] = {"values": {}, "variances": {}, "masks": {}}

            self.dslice = self.slice_data(array, slices)
            ydata = self.dslice.values
            xcenters = to_bin_centers(self.dslice.coords[self.dim],
                                      self.dim).values

            if self.hist[name]:
                new_values[name]["values"]["x"] = self.dslice.coords[
                    self.dim].values
                new_values[name]["values"]["y"] = np.concatenate(
                    (ydata[0:1], ydata))
            else:
                new_values[name]["values"]["x"] = xcenters
                new_values[name]["values"]["y"] = ydata
            if self.dslice.variances is not None:
                new_values[name]["variances"]["x"] = xcenters
                new_values[name]["variances"]["y"] = ydata
                new_values[name]["variances"]["e"] = vars_to_err(
                    self.dslice.variances)

            if len(mask_info[name]) > 0:
                base_mask = sc.Variable(dims=self.dslice.dims,
                                        values=np.ones(self.dslice.shape,
                                                       dtype=np.int32))
                for m in mask_info[name]:
                    # Use automatic broadcast to broadcast 0D masks
                    msk = (
                        base_mask *
                        sc.Variable(dims=self.dslice.masks[m].dims,
                                    values=self.dslice.masks[m].values.astype(
                                        np.int32))).values
                    if self.hist[name]:
                        msk = np.concatenate((msk[0:1], msk))

                    new_values[name]["masks"][m] = mask_to_float(
                        msk, new_values[name]["values"]["y"])

        return new_values

    def update_profile(self,
                       xdata=None,
                       ydata=None,
                       slices=None,
                       axparams=None,
                       mask_info=None):

        profile_dim = axparams["x"]["dim"]

        new_values = {}

        # Remove the current dim since it will be manually sliced below
        del slices[self.dim]

        # Find closest point to cursor
        # TODO: can we optimize this with new buckets?
        distance_to_cursor = np.abs(
            self.data_arrays[self.name].coords[self.dim].values - xdata)
        ind = np.argmin(distance_to_cursor)

        xcenters = to_bin_centers(
            self.data_arrays[self.name].coords[profile_dim],
            profile_dim).values

        for name, profile_slice in self.data_arrays.items():

            new_values[name] = {"values": {}, "variances": {}, "masks": {}}

            # Slice all dims apart from profile dim and currently displayed dim
            profile_slice = self.slice_data(profile_slice, slices)

            # Now slice the currently displayed dim
            profile_slice = profile_slice[self.dim, ind]

            ydata = profile_slice.values
            if axparams["x"]["hist"][name]:
                new_values[name]["values"]["x"] = profile_slice.coords[
                    profile_dim].values
                new_values[name]["values"]["y"] = np.concatenate(
                    (ydata[0:1], ydata))
            else:
                new_values[name]["values"]["x"] = xcenters
                new_values[name]["values"]["y"] = ydata
            if profile_slice.variances is not None:
                new_values[name]["variances"]["x"] = xcenters
                new_values[name]["variances"]["y"] = ydata
                new_values[name]["variances"]["e"] = vars_to_err(
                    profile_slice.variances)

            if len(mask_info[name]) > 0:
                base_mask = sc.Variable(dims=profile_slice.dims,
                                        values=np.ones(profile_slice.shape,
                                                       dtype=np.int32))
                for m in mask_info[name]:
                    # Use automatic broadcast to broadcast 0D masks
                    msk = (base_mask * sc.Variable(
                        dims=profile_slice.masks[m].dims,
                        values=profile_slice.masks[m].values.astype(
                            np.int32))).values
                    if axparams["x"]["hist"][name]:
                        msk = np.concatenate((msk[0:1], msk))

                    new_values[name]["masks"][m] = mask_to_float(
                        msk, new_values[name]["values"]["y"])

        return new_values
