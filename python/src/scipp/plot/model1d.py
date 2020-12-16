# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .model import PlotModel
from .tools import vars_to_err
import numpy as np


class PlotModel1d(PlotModel):
    """
    Model class for 1 dimensional plots.
    """
    def __init__(self, *args, **kwargs):

        super().__init__(*args, **kwargs)

        self.dim = None

        return

    def update_axes(self, axparams):
        """
        Update axes parameters on axis change.
        """
        self.dim = axparams["x"]["dim"]
        return

    def update_data(self, slices, mask_info):
        """
        Slice the data along dimension sliders that are not disabled for all
        entries in the dict of data arrays, and return a dict of 1d value
        arrays for data values, variances, and masks.
        """
        new_values = {}
        for name, array in self.data_arrays.items():
            new_values[name] = {"values": {}, "variances": {}, "masks": {}}
            self.dslice = self.slice_data(array, slices)
            new_values[name]["values"]["x"] = self.dslice.meta[self.dim].values
            new_values[name]["values"]["y"] = self.dslice.data.values
            if self.dslice.data.variances is not None:
                new_values[name]["variances"]["e"] = vars_to_err(
                    self.dslice.data.variances)
            new_values[name]["masks"] = self._make_masks(
                self.dslice, mask_info=mask_info[name])

        return new_values

    def update_profile(self,
                       xdata=None,
                       ydata=None,
                       slices=None,
                       axparams=None,
                       profile_dim=None,
                       mask_info=None):
        """
        Slice down all dimensions apart from the profile dimension, and send
        the data values, variances and masks back to the `PlotController`.
        """
        new_values = {}

        # Find closest point to cursor
        # TODO: can we optimize this with new buckets?
        distance_to_cursor = np.abs(
            self.data_arrays[self.name].meta[self.dim].values - xdata)
        ind = int(np.argmin(distance_to_cursor))

        for name, profile_slice in self.data_arrays.items():
            new_values[name] = {"values": {}, "variances": {}, "masks": {}}

            # Slice all dims apart from profile dim and currently displayed dim
            profile_slice = self.slice_data(profile_slice, slices)

            # Now slice the currently displayed dim
            profile_slice = profile_slice[self.dim, ind]
            new_values[name]["values"]["x"] = profile_slice.meta[
                profile_dim].values
            new_values[name]["values"]["y"] = profile_slice.data.values
            if profile_slice.data.variances is not None:
                new_values[name]["variances"]["e"] = vars_to_err(
                    profile_slice.data.variances)
            new_values[name]["masks"] = self._make_masks(
                profile_slice, mask_info=mask_info[name])

        return new_values
