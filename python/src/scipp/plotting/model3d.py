# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from functools import lru_cache
from .._scipp import core as sc
import numpy as np


def _planar_norm(a, b):
    return sc.sqrt(a * a + b * b)


class ScatterPointModel:
    def __init__(self, positions):
        self._axes = ['z', 'y', 'x']
        self._positions = positions

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
        return [
            0.5 * np.sum(self.limits['x']), 0.5 * np.sum(self.limits['y']),
            0.5 * np.sum(self.limits['z'])
        ]

    @property
    @lru_cache(maxsize=None)
    def box_size(self):
        return np.array([
            self.limits['x'][1] - self.limits['x'][0],
            self.limits['y'][1] - self.limits['y'][0],
            self.limits['z'][1] - self.limits['z'][0]
        ])

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

    @property
    @lru_cache(maxsize=None)
    def value(self):
        # TODO
        return sc.norm(self._positions)
