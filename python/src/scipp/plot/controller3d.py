# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .controller import PlotController
from .._utils import name_with_unit
import numpy as np


class PlotController3d(PlotController):
    """
    Controller class for 3d plots.

    It handles some additional events from the cut surface panel, compared to
    the base class controller.
    """
    def __init__(self, *args, pixel_size=None, positions=None, **kwargs):

        super().__init__(*args, initial_update=False, **kwargs)

        self.positions = positions
        self.pos_axparams = {}

        # If positions are specified, then the x, y, z points positions can
        # never change
        if self.positions is not None:
            extents = self.model.get_positions_extents(pixel_size)
            for xyz, ex in extents.items():
                self.pos_axparams[xyz] = {
                    "lims": ex["lims"],
                    "label": name_with_unit(1.0 * ex["unit"], name=xyz.upper())
                }

        # Call axes once to make the initial plot
        self.update_axes()
        self.update_log_axes_buttons()

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
            "update_cut_surface": self.update_cut_surface
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
            axparams = self.pos_axparams
        else:
            axparams = super()._get_axes_parameters()

        axparams["centre"] = [
            0.5 * np.sum(axparams['x']["lims"]),
            0.5 * np.sum(axparams['y']["lims"]),
            0.5 * np.sum(axparams['z']["lims"])
        ]

        axparams["box_size"] = np.array([
            axparams['x']["lims"][1] - axparams['x']["lims"][0],
            axparams['y']["lims"][1] - axparams['y']["lims"][0],
            axparams['z']["lims"][1] - axparams['z']["lims"][0]
        ])
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

    def rescale_to_data(self, button=None):
        """
        When we rescale the colorbar limits, we also have to manually update
        the colors in the figure, as opposed to Matplotlib's `imshow` which
        does the update automatically.
        """
        super().rescale_to_data()
        new_values = self.model.get_slice_values(
            mask_info=self.get_masks_info())
        self.view.update_data(new_values)

    def toggle_mask(self, change=None):
        """
        Show/hide masks
        """
        new_values = self.model.get_slice_values(
            mask_info=self.get_masks_info())
        self.view.update_data(new_values)

    def toggle_norm(self, change):
        """
        When toggling the color normalization, we need to get values from
        the model to update the colors in 3d plots.
        """
        super().toggle_norm(change)
        new_values = self.model.get_slice_values(
            mask_info=self.get_masks_info())
        self.view.update_data(new_values)
