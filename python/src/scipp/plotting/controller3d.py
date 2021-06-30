# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .controller import PlotController
from ..utils import name_with_unit
import numpy as np


class PlotController3d(PlotController):
    """
    Controller class for 3d plots.

    It handles some additional events from the cut surface panel, compared to
    the base class controller.
    """
    def __init__(self, *args, positions=None, **kwargs):

        super().__init__(*args, **kwargs)
        self.positions = positions

    def initialize_model(self):
        """
        Give the model3d the list of available options for the cut surface.
        """
        self.model.initialize(self.panel.get_cut_options())

    def _make_axes_parameters(self):
        """
        Gather the information (dimensions, limits, etc...) about the (x, y, z)
        axes that are displayed on the plots.
        If `positions` is specified, the axes never change and we simply return
        some axes parameters that were set upon creation.
        In addition, we give the center of the positions as half-way between
        the axes limits, as well as the extent of the positions which will be
        use to show an outline/box around the points in space.
        """
        if self.positions is not None:
            extents = self.model.get_positions_extents(self.pixel_size)
            # TODO replace by min and max calls on position components in fig
            axparams = {
                xyz: {
                    "lims": ex["lims"],
                    "label": name_with_unit(1.0 * ex["unit"],
                                            name=xyz.upper()),
                    "unit": name_with_unit(1.0 * ex["unit"], name="")
                }
                for xyz, ex in extents.items()
            }
        else:
            axparams = super()._make_axes_parameters()

        return axparams

    def get_axes_parameters(self):
        """
        Getter function for the current axes parameters.
        """
        return self.axparams

    def update_opacity(self, alpha):
        """
        When the opacity slider in the panel is changed, ask the view to update
        the opacity.
        """
        self.view.update_opacity(alpha=alpha)
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

    def update_cut_surface(self, *args, **kwargs):
        """
        When the position or thickness of the cut surface is changed via the
        widgets in the `PlotPanel3d`, get new alpha values from the
        `PlotModel3d` and send them to the `PlotView3d` for updating the color
        array.
        """
        alpha = self.model.update_cut_surface(*args, **kwargs)
        self.view.update_opacity(alpha=alpha)

    def get_pixel_size(self):
        """
        Getter function for the pixel size.
        """
        return self.axparams["pixel_size"] / self.axparams["pixel_scaling"]
