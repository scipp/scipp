# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet
from ..core import abs as abs_
from .controller import PlotController


class PlotController3d(PlotController):
    """
    Controller class for 3d plots.

    It handles some additional events from the cut surface panel, compared to
    the base class controller.
    """
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.view.set_position_params(self.model)
        for key in self.panel.options[:-1]:
            value = self._get_cut(key)
            self.panel.set_range(key, value.min(), value.max())

    def _get_cut(self, key):
        # PlotPanel3d currently uses hard-coded keys/labels for cut buttons
        dim = dict(zip(['x', 'y', 'z'], self.model.dims))
        if key == 'radius':
            return self.model.radius
        elif key in ['x', 'y', 'z']:
            return self.model.components[dim[key]]
        else:
            return self.model.planar_radius(axis=dim[key[7:]])

    def update_opacity(self, alpha):
        """
        When the opacity slider in the panel is changed, ask the view to update
        the opacity.
        """
        self.view.update_opacity(alpha=alpha)

    def remove_cut_surface(self):
        """
        When panel requests to remove the cut surface, ask the view to remove it.
        """
        self.view.remove_cut_surface()

    def add_cut_surface(self, *, key, center, delta, active, inactive):
        """
        When the position or thickness of the cut surface is changed via the
        widgets in the `PlotPanel3d`, get new alpha values from the
        `PlotModel3d` and send them to the `PlotView3d` for updating the color
        array.
        """
        if key == 'value':
            value = next(iter(self.view.data.values())).data
        else:
            value = self._get_cut(key)
        u = value.unit
        indices = (abs_(value - center * u) < 0.5 * delta * u).values
        self.view.add_cut_surface(indices)

    def get_pixel_size(self):
        """
        Getter function for the pixel size.
        """
        return self.view.figure._pixel_size / self.view.figure._pixel_scaling
