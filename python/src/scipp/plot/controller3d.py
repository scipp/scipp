# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .controller import PlotController
from .tools import to_bin_centers
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc

# Other imports
import numpy as np
# import ipywidgets as widgets
# from matplotlib import cm
# import matplotlib as mpl
# from matplotlib.backends import backend_agg
# import PIL as pil
# import pythreejs as p3
# from copy import copy



class PlotController3d(PlotController):

    def __init__(self,
                 scipp_obj_dict=None,
                 axes=None,
                 masks=None,
                 cmap=None,
                 log=None,
                 vmin=None,
                 vmax=None,
                 color=None,
                 logx=False,
                 logy=False,
                 logz=False,
                 button_options=None,
                 positions=None,
                 errorbars=None):

        super().__init__(scipp_obj_dict=scipp_obj_dict,
                 axes=axes,
                 masks=masks,
                 cmap=cmap,
                 log=log,
                 vmin=vmin,
                 vmax=vmax,
                 color=color,
                 logx=logx,
                 logy=logy,
                 logz=logz,
                 button_options=button_options,
                 # aspect=None,
                 positions=positions,
                 errorbars=errorbars)

        return

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

        # # Unfortunately, one cannot edit the value of the geometry array
        # # in-place, as this does not trigger an update on the threejs side.
        # # We have to update the entire array.
        # c3 = self.points_geometry.attributes["rgba_color"].array
        # c3[:, 3] = newc
        # self.points_geometry.attributes["rgba_color"].array = c3


    def rescale_to_data(self, button=None):
        vmin, vmax = self.model.rescale_to_data()
        self.panel.rescale_to_data(vmin, vmax)
        self.view.rescale_to_data(vmin, vmax)
        new_values = self.model.get_slice_values(mask_info=self.get_mask_info())
        self.view.update_data(new_values)

    def toggle_mask(self, change=None):
        """
        Show/hide masks
        """
        new_values = self.model.get_slice_values(mask_info=self.get_mask_info())
        self.view.update_data(new_values)
        return





