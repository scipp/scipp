# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .model import PlotModel
from .tools import find_limits
import numpy as np


class PlotModel1d(PlotModel):
    """
    Model class for 1 dimensional plots.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        for name, array in self.data_arrays.items():
            if array.bins is not None:
                raise RuntimeError(
                    "1-D plots of binned data not implemented. Histogram "
                    "manually, e.g., using `plot(array.bins.sum())`")
        self.dim = None

    def update_axes(self, axparams):
        """
        Update axes parameters on axis change.
        """
        self.dim = axparams["x"]["dim"]

    def update_data(self, slices, mask_info):
        """
        Slice the data along dimension sliders that are not disabled for all
        entries in the dict of data arrays, and return a dict of 1d value
        arrays for data values, variances, and masks.
        """
        self.dslice = {}
        out = {}
        for name in self.data_arrays:
            self.dslice[name] = self.slice_data(self.data_arrays[name], slices)
            out[name] = self._make_profile(self.dslice[name], self.dim,
                                           mask_info[name])
        return out

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
        # Find closest point to cursor
        # TODO: can we optimize this with new buckets?
        distance_to_cursor = np.abs(
            self.data_arrays[self.name].meta[self.dim].values - xdata)
        ind = int(np.argmin(distance_to_cursor))
        # Slice all dims apart from profile dim and currently displayed dim,
        # then currently displayed dim.
        return {
            name: self._make_profile(
                self.slice_data(self.data_arrays[name], slices)[self.dim, ind],
                profile_dim, mask_info[name])
            for name in self.data_arrays
        }

    def rescale_to_data(self, scale=None):
        """
        Get the min and max values of the currently displayed slice.
        """
        if self.dslice is not None:
            limits = np.array([
                find_limits(array.data, scale=scale)[scale]
                for array in self.dslice.values()
            ])
            return [np.amin(limits[:, 0]), np.amax(limits[:, 1])]
        else:
            return [None, None]
