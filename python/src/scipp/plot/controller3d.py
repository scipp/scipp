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
                 errorbars=None,
            pixel_size=None):

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

        self.positions = positions
        self.pos_axparams = {}

        # If positions are specified, then the x, y, z points positions can
        # never change
        if self.positions is not None:
            coord = scipp_obj_dict[self.name].coords[self.positions]
            # self.pos_array = np.array(coord.values, dtype=np.float32)
            # self.pos_array = scipp_obj_dict[self.name].coords[self.positions].values.astype(np.float32)
            for xyz in "xyz":
                x = getattr(sc.geometry, xyz)(coord)
                self.pos_axparams[xyz] = {"lims": [sc.min(x).value - 0.5 * pixel_size,
                                         sc.max(x).value + 0.5 * pixel_size],
                                         "label": name_with_unit(coord, name=xyz.upper())}


        return



    def get_axes_parameters(self):
        axparams = {}
        if self.positions is not None:
            # # axparams = {}
            # # coord = scipp_obj_dict[self.name].coords[self.positions]
            # # self.pos_array = np.array(coord.values, dtype=np.float32)
            # for xyz in "xyz":
            #     # x = getattr(sc.geometry, xyz)(coord)
            #     axparams[xyz] = {}
            #     axparams[xyz]["lims"] = self.model.get_positions_extents(xyz)
            #     axparams[xyz]["label"] = name_with_unit(
            #         self.data_arrays[self.name].coords[self.positions], name=xyz.upper())
            # # self.axparams["pos"] = self.pos_array
            # # retaxparams
            axparams = self.pos_axparams
        else:
            axparams = super().get_axes_parameters()
        #     # If no positions are supplied, create a meshgrid from coordinate
        #     # axes.
        #     coords = {}
        #     # labels = []
        #     for dim, button in self.widgets.buttons.items():
        #         if self.widgets.slider[dim].disabled:
        #             # for dim in limits:
        #             # xyz = limits[dim]["button"]
        #             xyz = button.value.lower()
        #             # coord = self.data_arrays[self.name].coords[dim]
        #             # self.axparams[xyz]["coord"] = to_bin_centers(coord, dim)#.values
        #             # axparams[xyz]["coord"] = self.data_arrays[self.name].coords[dim]
        #             axparams[xyz]["dim"] = dim
        #             # self.axparams[xyz]["shape"] = self.axparams[xyz]["coord"].shape

        #             # labels.append(name_with_unit(coord))
        #             self.axparams[xyz]["labels"] = name_with_unit(self.axparams[xyz]["coord"])
        #             self.axparams[xyz]["lims"] = limits[dim]["xlims"]
        #     z, y, x = np.meshgrid(to_bin_centers(self.axparams['z']["coord"], self.axparams["z"]["dim"]).values,
        #                           to_bin_centers(self.axparams['y']["coord"], self.axparams["y"]["dim"]).values,
        #                           to_bin_centers(self.axparams['x']["coord"], self.axparams["x"]["dim"]).values,
        #                           indexing='ij')
        #     # x, y, z = np.meshgrid(*coords, indexing='ij')
        #     self.pos_array = np.array(
        #         [x.ravel(), y.ravel(), z.ravel()], dtype=np.float32).T


        print(axparams)
        axparams["centre"] = [
            0.5 * np.sum(axparams['x']["lims"]), 0.5 * np.sum(axparams['y']["lims"]),
            0.5 * np.sum(axparams['z']["lims"])
        ]

        axparams["box_size"] = np.array([
            axparams['x']["lims"][1] - axparams['x']["lims"][0],
            axparams['y']["lims"][1] - axparams['y']["lims"][0],
            axparams['z']["lims"][1] - axparams['z']["lims"][0]])

        # axparams["pos"] = self.model.get_positions_array()

        return axparams

    def get_positions_array(self):
        return self.model.get_positions_array()





        # axparams = {}
        # for dim, button in self.widgets.buttons.items():
        #     if self.widgets.slider[dim].disabled:
        #         but_val = button.value.lower()
        #         xmin = np.Inf
        #         xmax = np.NINF
        #         for name in self.xlims:
        #             xlims = self.xlims[name][dim].values
        #             xmin = min(xmin, xlims[0])
        #             xmax = max(xmax, xlims[1])
        #         axparams[but_val] = {
        #             "lims": [xmin, xmax],
        #             "log": getattr(self, "log{}".format(but_val)),
        #             "hist": {name: self.histograms[name][dim][dim] for name in self.histograms},
        #             "dim": dim,
        #             "label": self.labels[self.name][dim]
        #         }
        #         # Safety check for log axes
        #         if axparams[but_val]["log"] and (axparams[but_val]["lims"][0] <= 0):
        #             axparams[but_val]["lims"][
        #                 0] = 1.0e-03 * axparams[but_val]["lims"][1]

        #         # limits[dim] = {"button": but_val,
        #         # "xlims": self.xlims[self.name][dim].values,
        #         # "log": getattr(self, "log{}".format(but_val)),
        #         # "hist": {name: self.histograms[name][dim][dim] for name in self.histograms}}
        # return axparams

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





