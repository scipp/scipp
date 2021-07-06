# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from functools import lru_cache
from .._scipp import core as sc
from .._shape import flatten
from .model1d import PlotModel1d
from .model import DataArrayDict
import numpy as np


def _planar_norm(a, b):
    return sc.sqrt(a * a + b * b)


class ScatterPointModel:
    """
    Model representing scattered data.
    """
    def __init__(self, *, positions, scipp_obj_dict, resolution):
        self._axes = ['z', 'y', 'x']
        # TODO use resolution=None?
        self._data_model = PlotModel1d(scipp_obj_dict=scipp_obj_dict,
                                       resolution=resolution)
        array = next(iter(scipp_obj_dict.values()))
        if positions is None:
            self._make_components(dims=array.dims[-3:])
        else:
            # TODO Get dim labels from field names
            self._scatter_dims = ['x', 'y', 'z']
            self._positions = flatten(array.meta[positions],
                                      to=''.join(array.dims))
            self._components = {'x': self.x, 'y': self.y, 'z': self.z}

    def update_data(self, slices):
        arrays = self._data_model.update_data(slices=slices)
        return DataArrayDict({
            key: flatten(array, to=''.join(array.dims))
            for key, array in arrays.items()
        })

    def _make_components(self, dims):
        array = next(iter(self._data_model.data_arrays.values()))
        slice_dims = [dim for dim in array.dims if dim not in dims]
        self._scatter_dims = dims
        for dim in slice_dims:
            array = array[dim, 0]
        array = flatten(array, to=''.join(array.dims))
        self._components = {dim: array.meta[dim] for dim in self._scatter_dims}
        comps = []
        for field in self._components.values():
            comp = field.astype(sc.dtype.float64).copy()
            comp.unit = ''
            comps.append(comp)
        self._positions = sc.geometry.position(*comps)

    @property
    def dims(self):
        return self._scatter_dims

    @dims.setter
    def dims(self, dims):
        self._make_components(dims)

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
        pos = self._positions.fields
        for xyz, x in zip(self._axes, [pos.z, pos.y, pos.x]):
            xmin = sc.min(x).value
            xmax = sc.max(x).value
            extents[xyz] = np.array([xmin, xmax])
        return extents

    @property
    @lru_cache(maxsize=None)
    def center(self):
        return np.array([0.5 * np.sum(self.limits[dim]) for dim in 'xyz'])

    @property
    @lru_cache(maxsize=None)
    def box_size(self):
        return np.array([
            self.limits['x'][1] - self.limits['x'][0],
            self.limits['y'][1] - self.limits['y'][0],
            self.limits['z'][1] - self.limits['z'][0]
        ])

    # TODO replace x,y,z? use dims
    @property
    @lru_cache(maxsize=None)
    def components(self):
        return self._components

    @property
    @lru_cache(maxsize=None)
    def x(self):
        return self._positions.fields.x

    @property
    @lru_cache(maxsize=None)
    def y(self):
        return self._positions.fields.y

    @property
    @lru_cache(maxsize=None)
    def z(self):
        return self._positions.fields.z

    @property
    @lru_cache(maxsize=None)
    def radius_x(self):
        return _planar_norm(self._positions.fields.y, self._positions.fields.z)

    @property
    @lru_cache(maxsize=None)
    def radius_y(self):
        return _planar_norm(self._positions.fields.x, self._positions.fields.z)

    @property
    @lru_cache(maxsize=None)
    def radius_z(self):
        return _planar_norm(self._positions.fields.x, self._positions.fields.y)

    @property
    @lru_cache(maxsize=None)
    def radius(self):
        return sc.norm(self._positions)

    def __getattr__(self, attr):
        """
        Forward some methods from internal PlotModel1d.
        """
        return getattr(self._data_model, attr)
