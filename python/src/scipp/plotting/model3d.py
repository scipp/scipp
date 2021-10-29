# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from functools import lru_cache
from ..core import flatten, sqrt, norm
from .model1d import PlotModel1d
from .resampling_model import ResamplingMode
import numpy as np


def _planar_norm(a, b):
    return sqrt(a * a + b * b)


def _flatten(da, *, dims, mode=None):
    flat = flatten(da, dims=dims, to='_'.join(dims))
    if flat.bins is None:
        return flat
    mode = ResamplingMode(mode)
    if mode == ResamplingMode.mean:
        return flat.bins.mean()
    else:
        return flat.bins.sum()


class ScatterPointModel:
    """
    Model representing scattered data.
    """
    def __init__(self, *, positions, scipp_obj_dict, resolution):
        self._scipp_obj_dict = scipp_obj_dict
        self._positions_dims = positions.dims
        self._resolution = resolution
        self._positions = _flatten(positions, dims=self._positions_dims)
        # TODO Get dim labels from field names
        self._scatter_dims = ['x', 'y', 'z']
        fields = self._positions.fields
        self._components = dict(zip(self.dims, [fields.x, fields.y, fields.z]))

    def _mode_updated(self):
        self._data_model.mode = self.mode

    @property
    def is_resampling(self):
        return next(iter(self._scipp_obj_dict.values())).bins is not None

    def _initialize(self):
        scipp_obj_dict = {
            key: _flatten(array, dims=self._positions_dims, mode=self.mode)
            for key, array in self._scipp_obj_dict.items()
        }
        self._data_model = PlotModel1d(scipp_obj_dict=scipp_obj_dict,
                                       resolution=self._resolution)
        self._data_model.mode = self._mode

    @property
    def dims(self):
        return self._scatter_dims

    @property
    def positions(self):
        return self._positions

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, m: ResamplingMode):
        self._mode = m
        self._initialize()

    @property
    def unit(self):
        return self._positions.unit

    @property
    @lru_cache(maxsize=None)
    def limits(self):
        """
        Extents of the box that contains all the positions.
        """
        extents = {}
        for dim, x in self.components.items():
            xmin = x.min().value
            xmax = x.max().value
            extents[dim] = np.array([xmin, xmax])
        return extents

    @property
    @lru_cache(maxsize=None)
    def center(self):
        return np.array([0.5 * np.sum(self.limits[dim]) for dim in self.dims])

    @property
    @lru_cache(maxsize=None)
    def box_size(self):
        return np.array(
            [self.limits[dim][1] - self.limits[dim][0] for dim in self.dims])

    @property
    @lru_cache(maxsize=None)
    def components(self):
        return self._components

    @lru_cache(maxsize=None)
    def planar_radius(self, axis):
        return _planar_norm(
            *[comp for dim, comp in self.components.items() if dim is not axis])

    @property
    @lru_cache(maxsize=None)
    def radius(self):
        return norm(self._positions)

    def __getattr__(self, attr):
        """
        Forward some methods from internal PlotModel1d.
        """
        return getattr(self._data_model, attr)
