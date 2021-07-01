# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .._variable import zeros
from .view import PlotView
import numpy as np


class PlotView3d(PlotView):
    """
    View object for 3 dimensional plots. Contains a `PlotFigure3d`.

    The view also handles events to do with updating opacities of the cut
    surface.

    This will also be handling profile picking events in the future.
    """
    def __init__(self, figure, formatters):
        super().__init__(figure=figure, formatters=formatters)
        self._axes = ['z', 'y', 'x']
        self.cut_options = None

    def update_opacity(self, *args, **kwargs):
        self.figure.update_opacity(*args, **kwargs)

    def update_depth_test(self, *args, **kwargs):
        self.figure.update_depth_test(*args, **kwargs)

    def close(self):
        """
        Dummy close function because 3d plots cannot be closed like mpl
        figures.
        """
        return

    def _make_data(self, new_values, mask_info):
        # TODO could handle multiple clouds here?
        array = next(iter(new_values.values()))
        new_values = {'data': array}
        mask_info = next(iter(mask_info.values()))
        if len(mask_info) > 0:
            # Use automatic broadcasting in Scipp variables
            msk = zeros(sizes=array.sizes, dtype='int32')
            for m, val in mask_info.items():
                if val:
                    msk += array.masks[m].astype(msk.dtype)
            new_values['mask'] = msk
        return new_values

    def update_data(self, new_values, mask_info=None):
        """
        Forward data update to the `figure`.
        """
        self._data = new_values
        # TODO In principle we should use actual dimension here, usually x,y,z
        self.figure.toolbar.dims = []
        self.refresh(mask_info)

    def set_position_params(self, params):
        self._positions = params.positions
        self.figure.set_position_params(params)

    def update_cut_surface(self,
                           target=None,
                           button_value=None,
                           surface_thickness=None,
                           opacity_lower=None,
                           opacity_upper=None):
        """
        Compute new opacities based on positions of the cut surface.
        """
        array = next(iter(self._data.values()))
        pos_array = self._positions.values

        # Cartesian X, Y, Z
        if button_value < self.cut_options["Xcylinder"]:
            return np.where(
                np.abs(pos_array[:, button_value] - target) <
                0.5 * surface_thickness, opacity_upper, opacity_lower)
        # Cylindrical X, Y, Z
        elif button_value < self.cut_options["Sphere"]:
            axis = button_value - 3
            remaining_inds = [(axis + 1) % 3, (axis + 2) % 3]
            return np.where(
                np.abs(
                    np.sqrt(pos_array[:, remaining_inds[0]] *
                            pos_array[:, remaining_inds[0]] +
                            pos_array[:, remaining_inds[1]] *
                            pos_array[:, remaining_inds[1]]) - target) <
                0.5 * surface_thickness, opacity_upper, opacity_lower)
        # Spherical
        elif button_value == self.cut_options["Sphere"]:
            return np.where(
                np.abs(
                    np.sqrt(pos_array[:, 0] * pos_array[:, 0] +
                            pos_array[:, 1] * pos_array[:, 1] +
                            pos_array[:, 2] * pos_array[:, 2]) - target) <
                0.5 * surface_thickness, opacity_upper, opacity_lower)
        # Value iso-surface
        elif button_value == self.cut_options["Value"]:
            return np.where(
                np.abs(array.values.ravel() - target) <
                0.5 * surface_thickness, opacity_upper, opacity_lower)
        else:
            raise RuntimeError(
                "Unknown cut surface type {}".format(button_value))
