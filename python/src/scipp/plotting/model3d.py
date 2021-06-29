# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .tools import fix_empty_range
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


def get_positions_extents(self, pixel_size=None):
    """
    Find the extents of the box that contains all the positions.
    """
    extents = {}
    pos = self.pos_coord.fields
    for xyz, x in zip(['x', 'y', 'z'], [pos.x, pos.y, pos.z]):
        xmin = sc.min(x).value
        xmax = sc.max(x).value
        if pixel_size is not None:
            xmin -= 0.5 * pixel_size
            xmax += 0.5 * pixel_size
        extents[xyz] = {
            "lims":
            np.array(fix_empty_range([xmin, xmax], replacement=pixel_size)),
            "unit": self.pos_coord.unit
        }
    return extents


def estimate_pixel_size(self, axparams):
    """
    Find the smallest pixel in the grid.
    """
    dx = [
        axparams["box_size"][i] /
        self.data_arrays[self.name].sizes[axparams[xyz]["dim"]]
        for i, xyz in enumerate("xyz")
    ]
    scaling = [axparams[xyz]["scaling"] for xyz in "xyz"]
    ind = np.argmin(dx)
    return dx[ind], scaling[ind]
