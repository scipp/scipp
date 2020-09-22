# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .controller import PlotController
from .._utils import name_with_unit
from .._scipp import core as sc

# Other imports
import numpy as np


class PlotController3d(PlotController):
    def __init__(self,
                 scipp_obj_dict=None,
                 pixel_size=None,
                 positions=None,
                 **kwargs):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                         positions=positions,
                         **kwargs)

        self.positions = positions
        self.pos_axparams = {}

        # If positions are specified, then the x, y, z points positions can
        # never change
        if self.positions is not None:
            coord = scipp_obj_dict[self.name].coords[self.positions]
            for xyz in "xyz":
                x = getattr(sc.geometry, xyz)(coord)
                self.pos_axparams[xyz] = {
                    "lims": [
                        sc.min(x).value - 0.5 * pixel_size,
                        sc.max(x).value + 0.5 * pixel_size
                    ],
                    "label":
                    name_with_unit(coord, name=xyz.upper())
                }

    def get_axes_parameters(self):
        axparams = {}
        if self.positions is not None:
            axparams = self.pos_axparams
        else:
            axparams = super().get_axes_parameters()

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

    def get_positions_array(self):
        return self.model.get_positions_array()

    def update_opacity(self, alpha):
        self.view.update_opacity(alpha=alpha)
        # There is a strange effect with point clouds and opacities.
        # Results are best when depthTest is False, at low opacities.
        # But when opacities are high, the points appear in the order
        # they were drawn, and not in the order they are with respect
        # to the camera position. So for high opacities, we switch to
        # depthTest = True.
        self.view.update_depth_test(alpha > 0.9)

    def update_depth_test(self, value):
        self.view.update_depth_test(value)

    def update_cut_surface(self, **kwargs):
        alpha = self.model.update_cut_surface(**kwargs)
        self.view.update_opacity(alpha=alpha)

    def rescale_to_data(self, button=None):
        vmin, vmax = self.model.rescale_to_data()
        self.panel.rescale_to_data(vmin, vmax)
        self.view.rescale_to_data(vmin, vmax)
        new_values = self.model.get_slice_values(
            mask_info=self.get_mask_info())
        self.view.update_data(new_values)

    def toggle_mask(self, change=None):
        """
        Show/hide masks
        """
        new_values = self.model.get_slice_values(
            mask_info=self.get_mask_info())
        self.view.update_data(new_values)
