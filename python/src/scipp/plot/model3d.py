# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .. import config
from .model import PlotModel
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



class PlotModel3d(PlotModel):

    def __init__(self,
                 controller=None,
                 scipp_obj_dict=None,
                 positions=None,
                 cut_options=None):

        super().__init__(controller=controller,
                         scipp_obj_dict=scipp_obj_dict)



        self.dslice = None
        # Useful variables
        # self.permutations = {"x": ["y", "z"], "y": ["x", "z"], "z": ["x", "y"]}
        # self.remaining_inds = [0, 1]

        self.axparams = {"x": {}, "y": {}, "z": {}, "pos": None}
        self.positions = positions
        self.pos_array = None

        self.cut_options = cut_options


        # # If positions are specified, then the x, y, z points positions can
        # # never change
        # if self.positions is not None:
        #     coord = scipp_obj_dict[self.name].coords[self.positions]
        #     self.pos_array = np.array(coord.values, dtype=np.float32)
        #     for xyz in "xyz":
        #         x = getattr(sc.geometry, xyz)(coord)
        #         self.axparams[xyz]["lims"] = [sc.min(x).value - 0.5 * pixel_size, sc.max(x).value + 0.5 * pixel_size]
        #         self.axparams[xyz]["labels"] = name_with_unit(coord, name=xyz.upper())
        #     self.axparams["pos"] = self.pos_array

        return



    def update_axes(self, limits):

        # If positions are specified, then the x, y, z points positions can
        # never change
        if self.positions is not None:
            coord = scipp_obj_dict[self.name].coords[self.positions]
            self.pos_array = np.array(coord.values, dtype=np.float32)
            for xyz in "xyz":
                x = getattr(sc.geometry, xyz)(coord)
                self.axparams[xyz]["lims"] = [sc.min(x).value - 0.5 * pixel_size, sc.max(x).value + 0.5 * pixel_size]
                self.axparams[xyz]["labels"] = name_with_unit(coord, name=xyz.upper())
            # self.axparams["pos"] = self.pos_array
        else:
            # If no positions are supplied, create a meshgrid from coordinate
            # axes.
            coords = {}
            # labels = []
            for dim in limits:
                xyz = limits[dim]["button"]
                # coord = self.data_arrays[self.name].coords[dim]
                # self.axparams[xyz]["coord"] = to_bin_centers(coord, dim)#.values
                self.axparams[xyz]["coord"] = self.data_arrays[self.name].coords[dim]
                self.axparams[xyz]["dim"] = dim
                # self.axparams[xyz]["shape"] = self.axparams[xyz]["coord"].shape

                # labels.append(name_with_unit(coord))
                self.axparams[xyz]["labels"] = name_with_unit(self.axparams[xyz]["coord"])
                self.axparams[xyz]["lims"] = limits[dim]["xlims"]
            z, y, x = np.meshgrid(to_bin_centers(self.axparams['z']["coord"], self.axparams["z"]["dim"]).values,
                                  to_bin_centers(self.axparams['y']["coord"], self.axparams["y"]["dim"]).values,
                                  to_bin_centers(self.axparams['x']["coord"], self.axparams["x"]["dim"]).values,
                                  indexing='ij')
            # x, y, z = np.meshgrid(*coords, indexing='ij')
            self.pos_array = np.array(
                [x.ravel(), y.ravel(), z.ravel()], dtype=np.float32).T
            # if self.pixel_size is None:
            #     self.pixel_size = coords[0][1] - coords[0][0]
            # self.axparams["x"]["labels"] = labels[0]
            # self.axparams["y"]["labels"] = labels[1]
            # self.axparams["z"]["labels"] = labels[2]
            # self.axlabels.update({
            #     "z": labels[0],
            #     "y": labels[1],
            #     "x": labels[2]
            # })

        self.axparams["pos"] = self.pos_array


        self.axparams["centre"] = [
            0.5 * np.sum(self.axparams['x']["lims"]), 0.5 * np.sum(self.axparams['y']["lims"]),
            0.5 * np.sum(self.axparams['z']["lims"])
        ]

        self.axparams["box_size"] = np.array([
            self.axparams['x']["lims"][1] - self.axparams['x']["lims"][0],
            self.axparams['y']["lims"][1] - self.axparams['y']["lims"][0],
            self.axparams['z']["lims"][1] - self.axparams['z']["lims"][0]])

        return self.axparams


    # def slice_data(self, change=None, autoscale_cmap=False):
    def slice_data(self, slices):
        """
        Slice the extra dimensions down and update the slice values
        """
        data_slice = self.data_arrays[self.name]
        # Slice along dimensions with active sliders
        for dim in slices:
        # for dim, val in self.parent.widgets.slider.items():
        #     if not val.disabled:
                # self.lab[dim].value = self.make_slider_label(
                #     self.slider_coord[self.name][dim], val.value)
                # self.lab[dim].value = self.make_slider_label(
                #     self.dslice.coords[dim], val.value, self.slider_axformatter[self.name][dim][False])
                # self.dslice = self.dslice[val.dim, val.value]

            deltax = slices[dim]["thickness"]
            loc = slices[dim]["location"]

            data_slice = self.resample_image(data_slice,
                    rebin_edges={dim: sc.Variable([dim], values=[loc - 0.5 * deltax,
                                                                 loc + 0.5 * deltax],
                                                        unit=data_slice.coords[dim].unit)})[dim, 0]
            data_slice *= (deltax * sc.units.one)



        shape = [self.axparams["z"]["coord"].shape[0] - 1,
                 self.axparams["y"]["coord"].shape[0] - 1,
                 self.axparams["x"]["coord"].shape[0] - 1]

        self.dslice = sc.DataArray(coords={
            self.axparams["z"]["dim"]: self.axparams["z"]["coord"],
            self.axparams["y"]["dim"]: self.axparams["y"]["coord"],
            self.axparams["x"]["dim"]: self.axparams["x"]["coord"]
            },
                                   data=sc.Variable(dims=[self.axparams["z"]["dim"],
                                    self.axparams["y"]["dim"],
                                    self.axparams["x"]["dim"]],
                                                    values=np.ones(shape),
                                                    variances=np.zeros(shape),
                                                    dtype=data_slice.dtype,
                                                    unit=sc.units.one))

        # print(self.dslice)
        # print("=======================")
        # print(data_slice)

        self.dslice *= data_slice

        # new_values = {"values": self.dslice.values, "masks": {}, "extent": extent}

        # # Handle masks
        # # if len(self.masks[self.name]) > 0:
        # msk = None
        # if len(self.parent.widgets.mask_checkboxes[self.name]) > 0:
        #     # Use automatic broadcasting in Scipp variables
        #     msk = sc.Variable(dims=self.dslice.dims,
        #                       values=np.zeros(self.dslice.shape,
        #                                       dtype=np.int32))
        #     for m, chbx in self.parent.widgets.mask_checkboxes[self.name].items():
        #         if chbx.value:
        #             msk += sc.Variable(
        #                 dims=self.dslice.masks[m].dims,
        #                 values=self.dslice.masks[m].values.astype(np.int32))
        #     msk = msk.values

        # self.dslice = self.dslice.values.flatten()
        # if autoscale_cmap:
        #     self.parent.scalar_map.set_clim(self.dslice.min(), self.dslice.max())
        # colors = self.parent.scalar_map.to_rgba(self.dslice).astype(np.float32)

        # if msk is not None:
        #     masks_inds = np.where(msk.flatten())
        #     masks_colors = self.parent.masks_scalar_map.to_rgba(
        #         self.dslice[masks_inds]).astype(np.float32)
        #     colors[masks_inds] = masks_colors

        # return colors

    def get_slice_values(self, mask_info):
        new_values = {"values": self.dslice.values.astype(np.float32).ravel(), "masks": None}

        # Handle masks
        msk = None
        if len(mask_info[self.name]) > 0:
            # Use automatic broadcasting in Scipp variables
            msk = sc.Variable(dims=self.dslice.dims,
                              values=np.zeros(self.dslice.shape,
                                              dtype=np.int32))
            for m, val in mask_info[self.name].items():
                if val:
                    msk += sc.Variable(
                        dims=self.dslice.masks[m].dims,
                        values=self.dslice.masks[m].values.astype(np.int32))
            new_values["masks"] = msk.values.ravel()
        return new_values

    def update_data(self, slices, mask_info):
        """
        Update colors of points.
        """
        self.slice_data(slices)

        return self.get_slice_values(mask_info)

        # new_values = {"values": self.dslice.values.astype(np.float32).ravel(), "masks": None}

        # # Handle masks
        # msk = None
        # if len(mask_info[self.name]) > 0:
        #     # Use automatic broadcasting in Scipp variables
        #     msk = sc.Variable(dims=self.dslice.dims,
        #                       values=np.zeros(self.dslice.shape,
        #                                       dtype=np.int32))
        #     for m, val in mask_info[self.name].items():
        #         if val:
        #             msk += sc.Variable(
        #                 dims=self.dslice.masks[m].dims,
        #                 values=self.dslice.masks[m].values.astype(np.int32))
        #     new_values["masks"] = msk.values.ravel()


        #     # msk = msk.values

        # # self.dslice = self.dslice.values.flatten()


        # # # if autoscale_cmap:
        # # #     self.parent.scalar_map.set_clim(self.dslice.min(), self.dslice.max())
        # # colors = self.parent.scalar_map.to_rgba(self.dslice).astype(np.float32)

        # # if msk is not None:
        # #     # In 3D, we change the colors of the points in-place where masks
        # #     # are True, instead of having an additional point cloud per mask.
        # #     masks_inds = np.where(msk.flatten())
        # #     masks_colors = self.parent.masks_scalar_map.to_rgba(
        # #         self.dslice[masks_inds]).astype(np.float32)
        # #     colors[masks_inds] = masks_colors
        # return new_values




    def update_cut_surface(self, target=None, button_value=None, surface_thickness=None,
        opacity_lower=None, opacity_upper=None):
        new_colors = None
        # target = self.cut_slider.value

        # Cartesian X, Y, Z
        if button_value < self.cut_options["Xcylinder"]:
            return np.where(
                np.abs(self.pos_array[:, button_value] -
                       target) < 0.5 * surface_thickness,
                opacity_upper, opacity_lower)
        # Cylindrical X, Y, Z
        elif button_value < self.cut_options["Sphere"]:
            axis = button_value - 3
            remaining_inds = [(axis + 1) % 3, (axis + 2) % 3]
            return np.where(
                np.abs(
                    np.sqrt(self.pos_array[:, remaining_inds[0]] *
                            self.pos_array[:, remaining_inds[0]] +
                            self.pos_array[:, remaining_inds[1]] *
                            self.pos_array[:, remaining_inds[1]]) -
                    target) < 0.5 * surface_thickness,
                opacity_upper, opacity_lower)
        # Spherical
        elif button_value == self.cut_options["Sphere"]:
            return np.where(
                np.abs(
                    np.sqrt(self.pos_array[:, 0] * self.pos_array[:, 0] +
                            self.pos_array[:, 1] * self.pos_array[:, 1] +
                            self.pos_array[:, 2] * self.pos_array[:, 2]) -
                    target) < 0.5 * surface_thickness,
                opacity_upper, opacity_lower)
        # Value iso-surface
        elif button_value == self.cut_options["Value"]:
            return np.where(
                np.abs(self.dslice.values.ravel() - target) <
                0.5 * surface_thickness,
                opacity_upper, opacity_lower)
        else:
            raise RuntimeError("Unknown cut surface type {}".format(button_value))




    # def update_opacity(self, change):
    #     """
    #     Update opacity of all points when opacity slider is changed.
    #     Take cut surface into account if present.
    #     """
    #     if self.parent.cut_surface_buttons.value is None:
    #         arr = self.parent.points_geometry.attributes["rgba_color"].array
    #         arr[:, 3] = change["new"][1]
    #         self.parent.points_geometry.attributes["rgba_color"].array = arr
    #         # There is a strange effect with point clouds and opacities.
    #         # Results are best when depthTest is False, at low opacities.
    #         # But when opacities are high, the points appear in the order
    #         # they were drawn, and not in the order they are with respect
    #         # to the camera position. So for high opacities, we switch to
    #         # depthTest = True.
    #         self.parent.points_material.depthTest = change["new"][1] > 0.9
    #     else:
    #         self.update_cut_surface({"new": self.parent.cut_slider.value})

    # def check_if_reset_needed(self, owner, content, buffers):
    #     if owner.value == self.current_cut_surface_value:
    #         self.parent.cut_surface_buttons.value = None
    #     self.current_cut_surface_value = owner.value

    # def update_cut_surface_buttons(self, change):
    #     if change["new"] is None:
    #         self.parent.cut_slider.disabled = True
    #         self.parent.cut_checkbox.disabled = True
    #         self.parent.cut_surface_thickness.disabled = True
    #         self.update_opacity({"new": self.parent.opacity_slider.value})
    #     else:
    #         self.parent.points_material.depthTest = False
    #         if change["old"] is None:
    #             self.parent.cut_slider.disabled = False
    #             self.parent.cut_checkbox.disabled = False
    #             self.parent.cut_surface_thickness.disabled = False
    #         self.update_cut_slider_bounds()

    # def update_cut_slider_bounds(self):
    #     # Cartesian X, Y, Z
    #     if self.parent.cut_surface_buttons.value < self.parent.cut_options["Xcylinder"]:
    #         minmax = self.parent.xminmax["xyz"[self.parent.cut_surface_buttons.value]]
    #         if minmax[0] < self.parent.cut_slider.max:
    #             self.parent.cut_slider.min = minmax[0]
    #             self.parent.cut_slider.max = minmax[1]
    #         else:
    #             self.parent.cut_slider.max = minmax[1]
    #             self.parent.cut_slider.min = minmax[0]
    #         self.parent.cut_slider.value = 0.5 * (minmax[0] + minmax[1])
    #     # Cylindrical X, Y, Z
    #     elif self.parent.cut_surface_buttons.value < self.parent.cut_options["Sphere"]:
    #         j = self.parent.cut_surface_buttons.value - 3
    #         remaining_axes = self.permutations["xyz"[j]]
    #         self.remaining_inds = [(j + 1) % 3, (j + 2) % 3]
    #         rmax = np.abs([
    #             self.parent.xminmax[remaining_axes[0]][0],
    #             self.parent.xminmax[remaining_axes[1]][0],
    #             self.parent.xminmax[remaining_axes[0]][1],
    #             self.parent.xminmax[remaining_axes[1]][1]
    #         ]).max()
    #         self.parent.cut_slider.min = 0
    #         self.parent.cut_slider.max = rmax * np.sqrt(2.0)
    #         self.parent.cut_slider.value = 0.5 * self.parent.cut_slider.max
    #     # Spherical
    #     elif self.parent.cut_surface_buttons.value == self.parent.cut_options["Sphere"]:
    #         rmax = np.abs(list(self.parent.xminmax.values())).max()
    #         self.parent.cut_slider.min = 0
    #         self.parent.cut_slider.max = rmax * np.sqrt(3.0)
    #         self.parent.cut_slider.value = 0.5 * self.parent.cut_slider.max
    #     # Value iso-surface
    #     elif self.parent.cut_surface_buttons.value == self.parent.cut_options["Value"]:
    #         self.parent.cut_slider.min = self.parent.vminmax[0]
    #         self.parent.cut_slider.max = self.parent.vminmax[1]
    #         self.parent.cut_slider.value = 0.5 * (self.parent.vminmax[0] + self.parent.vminmax[1])
    #         # Update slider step because it is no longer related to pixel size.
    #         # Slice thickness is linked to the step via jslink.
    #         self.parent.cut_slider.step = (self.parent.cut_slider.max -
    #                                 self.parent.cut_slider.min) / 10.0
    #     if self.parent.cut_surface_buttons.value < self.parent.cut_options["Value"]:
    #         self.parent.cut_slider.step = self.parent.pixel_size * 1.1

    # def update_cut_surface(self, change):
    #     newc = None
    #     target = self.parent.cut_slider.value
    #     # Cartesian X, Y, Z
    #     if self.parent.cut_surface_buttons.value < self.parent.cut_options["Xcylinder"]:
    #         newc = np.where(
    #             np.abs(self.parent.positions[:, self.parent.cut_surface_buttons.value] -
    #                    target) < 0.5 * self.parent.cut_surface_thickness.value,
    #             self.parent.opacity_slider.upper, self.parent.opacity_slider.lower)
    #     # Cylindrical X, Y, Z
    #     elif self.parent.cut_surface_buttons.value < self.parent.cut_options["Sphere"]:
    #         newc = np.where(
    #             np.abs(
    #                 np.sqrt(self.parent.positions[:, self.remaining_inds[0]] *
    #                         self.parent.positions[:, self.remaining_inds[0]] +
    #                         self.parent.positions[:, self.remaining_inds[1]] *
    #                         self.parent.positions[:, self.remaining_inds[1]]) -
    #                 target) < 0.5 * self.parent.cut_surface_thickness.value,
    #             self.parent.opacity_slider.upper, self.parent.opacity_slider.lower)
    #     # Spherical
    #     elif self.parent.cut_surface_buttons.value == self.parent.cut_options["Sphere"]:
    #         newc = np.where(
    #             np.abs(
    #                 np.sqrt(self.parent.positions[:, 0] * self.parent.positions[:, 0] +
    #                         self.parent.positions[:, 1] * self.parent.positions[:, 1] +
    #                         self.parent.positions[:, 2] * self.parent.positions[:, 2]) -
    #                 target) < 0.5 * self.parent.cut_surface_thickness.value,
    #             self.parent.opacity_slider.upper, self.parent.opacity_slider.lower)
    #     # Value iso-surface
    #     elif self.parent.cut_surface_buttons.value == self.parent.cut_options["Value"]:
    #         newc = np.where(
    #             np.abs(self.vslice - target) <
    #             0.5 * self.parent.cut_surface_thickness.value,
    #             self.parent.opacity_slider.upper, self.parent.opacity_slider.lower)

    #     # Unfortunately, one cannot edit the value of the geometry array
    #     # in-place, as this does not trigger an update on the threejs side.
    #     # We have to update the entire array.
    #     c3 = self.parent.points_geometry.attributes["rgba_color"].array
    #     c3[:, 3] = newc
    #     self.parent.points_geometry.attributes["rgba_color"].array = c3
