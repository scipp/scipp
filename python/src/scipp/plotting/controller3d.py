# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .._scipp import core as sc
from .._variable import scalar
from .controller import PlotController


class PlotController3d(PlotController):
    """
    Controller class for 3d plots.

    It handles some additional events from the cut surface panel, compared to
    the base class controller.
    """
    def __init__(self, *, position_model, **kwargs):
        super().__init__(**kwargs)
        self._position_model = position_model
        # TODO could be different for every axis
        self.view.set_position_params(self._position_model)
        for key in self.panel.options:
            # TODO vmin vmax
            value = getattr(self._position_model, key)
            self.panel.set_range(key, sc.min(value), sc.max(value))

    def initialize_model(self):
        pass

    def update_opacity(self, alpha):
        """
        When the opacity slider in the panel is changed, ask the view to update
        the opacity.
        """
        self.view.update_opacity(alpha=scalar(alpha))
        # There is a strange effect with point clouds and opacities.
        # Results are best when depthTest is False, at low opacities.
        # But when opacities are high, the points appear in the order
        # they were drawn, and not in the order they are with respect
        # to the camera position. So for high opacities, we switch to
        # depthTest = True.
        self.view.update_depth_test(alpha > 0.9)

    def update_depth_test(self, value):
        """
        Update the state of depth test in the view (see `update_opacity`).
        """
        self.view.update_depth_test(value)

    def update_cut_surface(self, *, key, center, delta, active, inactive):
        """
        When the position or thickness of the cut surface is changed via the
        widgets in the `PlotPanel3d`, get new alpha values from the
        `PlotModel3d` and send them to the `PlotView3d` for updating the color
        array.
        """
        if key == 'value':
            value = next(iter(self.view.data.values())).data
        else:
            value = getattr(self._position_model, key)
        u = value.unit
        alpha = sc.where(
            sc.abs(value - center * u) < 0.5 * delta * u, scalar(active),
            scalar(inactive))
        self.view.update_opacity(alpha=alpha)

    def get_pixel_size(self):
        """
        Getter function for the pixel size.
        """
        return self.view.figure._pixel_size / self.view.figure._pixel_scaling
