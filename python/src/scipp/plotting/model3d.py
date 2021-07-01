# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from .._scipp import core as sc
import numpy as np

# pos = [xyz]
# dims = [xyzab]
#
# model1d: 4 sliders (xyza), indices provided in update_data
# modelX: 2 sliders (ab), flatten xyz => row
#
# 1. flatten xyz => row: dims = [rab]
# 2. model1d: 2 sliders (ab)

# figure: display 1d table with x,y,z columns (3d scatter plot)
# what would we change to also make the same design work for 2d scatter plot?
# Figure3dScatter: expect 1d table with x,y,z columns
# Figure2dScatter: expect 1d table with x,y columns
# Model: wrap Nd data, slice/resample some
# don't flatten at all? can do plot without pos by manually flattening and zipping coords
# 3d scatter plot dims defined as: data.dims - pos.dims + [x,y,z]
# 2d scatter plot dims defined as: data.dims - pos.dims + [x,y]

# objects.py dim_label_map => widgets.py slider setup
# - should not make slider for pos dims
# - creator of PlotWidgets should create with correct dims

# PlotModel1d is actually Nd, unless resampling
#
# PlotWidgets: Care only about non-scatter dims
# PlotModel3d: Ignore scatter dims, slice/resample others, how to define dims of model?
# => ScatterPlotModel, no dims?
# PlotFigure3d: dims = scatter-dims (x,y,z)
# Plot: data.dims - pos.dims + [x,y,z]

# - Toolbar


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
    def center(self):
        return [
            0.5 * np.sum(self.limits['x']), 0.5 * np.sum(self.limits['y']),
            0.5 * np.sum(self.limits['z'])
        ]

    @property
    def box_size(self):
        return np.array([
            self.limits['x'][1] - self.limits['x'][0],
            self.limits['y'][1] - self.limits['y'][0],
            self.limits['z'][1] - self.limits['z'][0]
        ])
