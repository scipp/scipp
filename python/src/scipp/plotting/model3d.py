# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from functools import lru_cache
from .._scipp import core as sc
from .._shape import flatten
from .model1d import PlotModel1d
import numpy as np


def _planar_norm(a, b):
    return sc.sqrt(a * a + b * b)


def _flatten(da, dims):
    return flatten(da, dims=dims, to='_'.join(dims))


class ScatterPointModel:
    """
    Model representing scattered data.
    """
    def __init__(self, *, positions, scipp_obj_dict, resolution):
        scipp_obj_dict = {
            key: _flatten(array, dims=positions.dims)
            for key, array in scipp_obj_dict.items()
        }
        self._data_model = PlotModel1d(scipp_obj_dict=scipp_obj_dict,
                                       resolution=resolution)
        self._positions = _flatten(positions, dims=positions.dims)
        # TODO Get dim labels from field names
        self._scatter_dims = ['x', 'y', 'z']
        fields = self._positions.fields
        self._components = dict(zip(self.dims, [fields.x, fields.y, fields.z]))

    @property
    def dims(self):
        return self._scatter_dims

    @property
    def positions(self):
        return self._positions

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
            xmin = sc.min(x).value
            xmax = sc.max(x).value
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
        return sc.norm(self._positions)

    def __getattr__(self, attr):
        """
        Forward some methods from internal PlotModel1d.
        """
        return getattr(self._data_model, attr)
