# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .model import PlotModel, DataArrayDict
from .tools import find_limits
from .resampling_model import resampling_model
from .._scipp import core as sc
from ..core import concat


class PlotModel1d(PlotModel):
    """
    Model class for 1 dimensional plots.
    """
    def __init__(self, *args, resolution=None, **kwargs):
        super().__init__(*args, **kwargs)
        self._resolution = resolution
        for name, array in self.data_arrays.items():
            if array.bins is not None and self._resolution is None:
                self._resolution = 200
            self.dims = array.dims[-1:]

    def _make_1d_resampling_model(self, array):
        model = resampling_model(array)
        model.mode = self.mode
        if self.resolution is not None:
            model.resolution[self.dims[0]] = self.resolution
            model.bounds[self.dims[0]] = None
        return model

    def _resample(self, array, slices):
        model = self._make_1d_resampling_model(array)
        # Extend slice to full input "pixel" sizes
        for dim in slices:
            # TODO Unfortunately we cannot extend in this manner when there
            # are 2-d coords
            if len(model._array.meta[dim].dims) != 1:
                continue
            if not isinstance(slices[dim], tuple) or not isinstance(
                    slices[dim][0], sc.Variable):
                continue
            start, stop = slices[dim]
            s = sc.get_slice_params(model._array.data, model._array.meta[dim], start,
                                    stop)[1]
            if s.start + 1 == s.stop:
                slices[dim] = s.start
            else:
                slices[dim] = (s.start, s.stop)
        model.bounds.update(slices)
        return model.data

    @property
    def is_resampling(self):
        # Two relevant cases where ndim != 1 leads to resampling:
        # - Profile plot of higher-dimensional data.
        # - projection='1d', with non-trivial thickness (set via slider).
        # In the latter case there is strictly speaking no resampling if thickness is 1
        # but in that case sum==mean and we avoid hiding/showing the toolbar button
        # depending on selected thickness.
        return self._resolution is not None or len(self.data_arrays.dims) != 1

    @property
    def resolution(self):
        return self._resolution

    @resolution.setter
    def resolution(self, resolution):
        self._resolution = resolution

    def update(self):
        """
        Create or update the internal resampling model.
        """
        # PlotModel1d currently creates ResamplingModel from scratch every
        # time update_data() is called, nothing to do here
        pass

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
        return DataArrayDict(self.dslice)

    def rescale_to_data(self, scale=None):
        """
        Get the min and max values of the currently displayed slice.
        """
        if self.dslice is not None:
            low = [
                find_limits(array.data, scale=scale)[scale][0]
                for array in self.dslice.values()
            ]
            high = [
                find_limits(array.data, scale=scale)[scale][1]
                for array in self.dslice.values()
            ]
            return [sc.min(concat(low, dim='dummy')), sc.max(concat(high, dim='dummy'))]
        else:
            return [None, None]
