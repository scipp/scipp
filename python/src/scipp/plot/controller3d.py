# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .. import config
from .controller import PlotController
from .._utils import name_with_unit
import numpy as np


class PlotController3d(PlotController):
    """
    Controller class for 3d plots.

    It handles some additional events from the cut surface panel, compared to
    the base class controller.
    """
    def __init__(self,
                 *args,
                 pixel_size=None,
                 positions=None,
                 aspect=None,
                 **kwargs):

        super().__init__(*args, **kwargs)
        self.positions = positions
        self.pixel_size = pixel_size
        self.aspect = aspect
        if self.aspect is None:
            if positions is not None:
                self.aspect = "equal"
            else:
                self.aspect = config.plot.aspect
        if self.aspect not in ["equal", "auto"]:
            raise RuntimeError(
                "Invalid aspect requested. Expected 'auto' or "
                "'equal', got", self.aspect)

    def initialise_model(self):
        """
        Give the model3d the list of available options for the cut surface.
        """
        self.model.initialise(self.panel.get_cut_options())

    def connect_panel(self):
        """
        Establish connection to the panel interface.
        """
        self.panel.connect({
            "update_opacity": self.update_opacity,
            "update_depth_test": self.update_depth_test,
            "update_cut_surface": self.update_cut_surface,
            "get_pixel_size": self.get_pixel_size
        })

    def _get_axes_parameters(self):
        """
        Gather the information (dimensions, limits, etc...) about the (x, y, z)
        axes that are displayed on the plots.
        If `positions` is specified, the axes never change and we simply return
        some axes parameters that were set upon creation.
        In addition, we give the centre of the positions as half-way between
        the axes limits, as well as the extent of the positions which will be
        use to show an outline/box around the points in space.
        """
        axparams = {}
        if self.positions is not None:
            extents = self.model.get_positions_extents()
            axparams = {
                xyz: {
                    "lims": ex["lims"],
                    "label": name_with_unit(1.0 * ex["unit"], name=xyz.upper())
                }
                for xyz, ex in extents.items()
            }
        else:
            axparams = super()._get_axes_parameters()

        axparams["box_size"] = np.array([
            axparams['x']["lims"][1] - axparams['x']["lims"][0],
            axparams['y']["lims"][1] - axparams['y']["lims"][0],
            axparams['z']["lims"][1] - axparams['z']["lims"][0]
        ])

        for i, xyz in enumerate("xyz"):
            axparams[xyz]["scaling"] = 1.0 / axparams["box_size"][
                i] if self.aspect == "auto" else 1.0
            axparams[xyz]["lims"] *= axparams[xyz]["scaling"]

        axparams["box_size"] *= np.array([
            axparams['x']["scaling"], axparams['y']["scaling"],
            axparams['z']["scaling"]
        ])

        axparams["centre"] = [
            0.5 * np.sum(axparams['x']["lims"]),
            0.5 * np.sum(axparams['y']["lims"]),
            0.5 * np.sum(axparams['z']["lims"])
        ]

        if self.pixel_size is not None:
            axparams["pixel_size"] = self.pixel_size
        else:
            if self.positions is not None:
                axparams["pixel_size"] = 0.05 * np.amin(axparams["box_size"])
            else:
                axparams["pixel_size"] = self.model.estimate_pixel_size(
                    axparams)

        return axparams

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
        return self.axparams["pixel_size"]
