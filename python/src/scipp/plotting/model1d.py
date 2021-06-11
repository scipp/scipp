# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .model import PlotModel
from .tools import find_limits
from .resampling_model import resampling_model
from .._scipp import core as sc


class PlotModel1d(PlotModel):
    """
    Model class for 1 dimensional plots.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.dim = None
        self._resolution = None
        for name, array in self.data_arrays.items():
            if array.bins is not None:
                self._resolution = 200

    def _make_1d_resampling_model(self, array):
        model = resampling_model(array)
        for dim in array.dims:
            if dim != self.dim:
                model.resolution[dim] = 1
            elif self.resolution is not None:
                model.resolution[dim] = self.resolution
                model.bounds[dim] = None
        return model

    def _resample(self, array, slices):
        model = self._make_1d_resampling_model(array)
        model.bounds.update(slices)
        data = model.data
        for dim in model.data.dims:
            if dim != self.dim:
                data = data[dim, 0]
        return data

    @property
    def resolution(self):
        return self._resolution

    @resolution.setter
    def resolution(self, resolution):
        self._resolution = resolution

    def update_axes(self, axparams):
        """
        Update axes parameters on axis change.
        """
        self.dim = axparams["x"]["dim"]

    def update_data(self, slices):
        """
        Slice the data along dimension sliders that are not disabled for all
        entries in the dict of data arrays, and return a dict of 1d value
        arrays for data values, variances, and masks.
        """
        self.dslice = {
            name: self._resample(array, slices)
            for name, array in self.data_arrays.items()
        }
        return self.dslice

    def rescale_to_data(self, scale=None):
        """
        Get the min and max values of the currently displayed slice.
        """
        from functools import reduce, partial
        if self.dslice is not None:
            low = [
                find_limits(array.data, scale=scale)[scale][0]
                for array in self.dslice.values()
            ]
            high = [
                find_limits(array.data, scale=scale)[scale][1]
                for array in self.dslice.values()
            ]
            return [
                sc.min(reduce(partial(sc.concatenate, dim='dummy'), low)),
                sc.max(reduce(partial(sc.concatenate, dim='dummy'), high))
            ]
        else:
            return [None, None]
