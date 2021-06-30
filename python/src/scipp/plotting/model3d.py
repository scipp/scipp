# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

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


def update_cut_surface(self,
                       target=None,
                       button_value=None,
                       surface_thickness=None,
                       opacity_lower=None,
                       opacity_upper=None):
    """
    Compute new opacities based on positions of the cut surface.
    """

    # Cartesian X, Y, Z
    if button_value < self.cut_options["Xcylinder"]:
        return np.where(
            np.abs(self.pos_array[:, button_value] - target) <
            0.5 * surface_thickness, opacity_upper, opacity_lower)
    # Cylindrical X, Y, Z
    elif button_value < self.cut_options["Sphere"]:
        axis = button_value - 3
        remaining_inds = [(axis + 1) % 3, (axis + 2) % 3]
        return np.where(
            np.abs(
                np.sqrt(self.pos_array[:, remaining_inds[0]] *
                        self.pos_array[:, remaining_inds[0]] +
                        self.pos_array[:, remaining_inds[1]] *
                        self.pos_array[:, remaining_inds[1]]) - target) <
            0.5 * surface_thickness, opacity_upper, opacity_lower)
    # Spherical
    elif button_value == self.cut_options["Sphere"]:
        return np.where(
            np.abs(
                np.sqrt(self.pos_array[:, 0] * self.pos_array[:, 0] +
                        self.pos_array[:, 1] * self.pos_array[:, 1] +
                        self.pos_array[:, 2] * self.pos_array[:, 2]) - target)
            < 0.5 * surface_thickness, opacity_upper, opacity_lower)
    # Value iso-surface
    elif button_value == self.cut_options["Value"]:
        return np.where(
            np.abs(self.dslice.data.values.ravel() - target) <
            0.5 * surface_thickness, opacity_upper, opacity_lower)
    else:
        raise RuntimeError("Unknown cut surface type {}".format(button_value))
