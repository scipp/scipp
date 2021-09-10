# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet, Simon Heybrock

from .. import config
from ..core import transpose
from .model import PlotModel
from .resampling_model import resampling_model


class PlotModel2d(PlotModel):
    """
    Model class for 2 dimensional plots.
    """
    def __init__(self, *args, resolution=None, **kwargs):
        self._model = None
        super().__init__(*args, **kwargs)
        if resolution is None:
            self._resolution = [config.plot.height, config.plot.width]
        elif isinstance(resolution, int):
            self._resolution = [resolution, resolution]
        else:
            self._resolution = [resolution[ax] for ax in 'yx']
        self.dims = next(iter(self.data_arrays.values())).dims[-2:]

    def _dims_updated(self):
        self._model.bounds = {}
        self._model.resolution = {}
        for dim, resolution in zip(self.dims, self._resolution):
            self._model.resolution[dim] = resolution
            self._model.bounds[dim] = None

    def _mode_updated(self):
        self._model.mode = self.mode

    @property
    def is_resampling(self):
        return True

    def update(self):
        """
        Create or update the internal resampling model.
        """
        if self._model is None:
            self._model = resampling_model(self.data_arrays[self.name])
        else:
            self._model.update_array(self.data_arrays[self.name])

    def update_data(self, slices):
        """
        Slice the data along dimension sliders that are not disabled for all
        entries in the dict of data arrays.
        Then perform dynamic image resampling based on current viewport.
        """
        for dim, bounds in slices.items():
            self._model.bounds[dim] = bounds
        self.dslice = self._model.data
        return transpose(self.dslice, dims=self._dims)

    def reset_resampling_model(self):
        self._model.reset()
