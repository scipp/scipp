# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .model import PlotModel
from .tools import to_bin_centers
from .._scipp import core as sc

# Other imports
import numpy as np


class PlotModel3d(PlotModel):
    def __init__(self,
                 *args,
                 positions=None,
                 # cut_options=None,
                 **kwargs):

        super().__init__(*args, **kwargs)

        # self.dslice = None
        self.button_dims = {}
        # self.dim_to_xyz = {}
        self.positions = positions
        self.pos_array = None
        self.pos_unit = None
        self.cut_options = None #cut_options

        # If positions are specified, then the x, y, z points positions can
        # never change
        if self.positions is not None:
            self.pos_array = np.array(
                scipp_obj_dict[self.name].coords[self.positions].values,
                dtype=np.float32)

    def initialise(self, cut_options):
        self.cut_options = cut_options


    # def get_positions_array(self):
    #     return self.pos_array

    def update_axes(self, axparams):

        # If no positions are supplied, create a meshgrid from coordinates
        if self.positions is None:

            for xyz in "zyx":
                self.button_dims[xyz] = axparams[xyz]["dim"]
                # self.dim_to_xyz[axparams[xyz]["dim"]] = xyz

            z, y, x = np.meshgrid(
                to_bin_centers(
                    self.data_arrays[self.name].coords[axparams['z']["dim"]],
                    axparams["z"]["dim"]).values,
                to_bin_centers(
                    self.data_arrays[self.name].coords[axparams['y']["dim"]],
                    axparams["y"]["dim"]).values,
                to_bin_centers(
                    self.data_arrays[self.name].coords[axparams['x']["dim"]],
                    axparams["x"]["dim"]).values,
                indexing='ij')

            self.pos_array = np.array(
                [x.ravel(), y.ravel(), z.ravel()], dtype=np.float32).T

        return {"positions": self.pos_array}

    def get_slice_values(self, mask_info):

        new_values = {
            "values": self.dslice.values.astype(np.float32).ravel(),
            "masks": None
        }

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
        Get new slice of data and send it back to the controller.
        """
        # self.slice_data(slices)
        data_slice = self.slice_data(self.data_arrays[self.name], slices)

        # Use automatic broadcast if positions are not used
        if self.positions is None:
            shape = [
                data_slice.coords[self.button_dims["z"]].shape[0] - 1,
                data_slice.coords[self.button_dims["y"]].shape[0] - 1,
                data_slice.coords[self.button_dims["x"]].shape[0] - 1
            ]

            self.dslice = sc.DataArray(coords={
                self.button_dims["z"]:
                data_slice.coords[self.button_dims["z"]],
                self.button_dims["y"]:
                data_slice.coords[self.button_dims["y"]],
                self.button_dims["x"]:
                data_slice.coords[self.button_dims["x"]]
            },
                                       data=sc.Variable(
                                           dims=[
                                               self.button_dims["z"],
                                               self.button_dims["y"],
                                               self.button_dims["x"]
                                           ],
                                           values=np.ones(shape),
                                           variances=np.zeros(shape),
                                           dtype=data_slice.dtype,
                                           unit=sc.units.one))

            self.dslice *= data_slice
        else:
            self.dslice = data_slice

        return self.get_slice_values(mask_info)

    def update_cut_surface(self,
                           target=None,
                           button_value=None,
                           surface_thickness=None,
                           opacity_lower=None,
                           opacity_upper=None):
        """
        Compute new opacities based on positions of the cut surface.
        """

        # Cartesian X, Y, Z
        if button_value < self.cut_options["Xcylinder"]:
            return np.where(
                np.abs(self.pos_array[:, button_value] - target) <
                0.5 * surface_thickness, opacity_upper, opacity_lower)
        # Cylindrical X, Y, Z
        elif button_value < self.cut_options["Sphere"]:
            axis = button_value - 3
            remaining_inds = [(axis + 1) % 3, (axis + 2) % 3]
            return np.where(
                np.abs(
                    np.sqrt(self.pos_array[:, remaining_inds[0]] *
                            self.pos_array[:, remaining_inds[0]] +
                            self.pos_array[:, remaining_inds[1]] *
                            self.pos_array[:, remaining_inds[1]]) - target) <
                0.5 * surface_thickness, opacity_upper, opacity_lower)
        # Spherical
        elif button_value == self.cut_options["Sphere"]:
            return np.where(
                np.abs(
                    np.sqrt(self.pos_array[:, 0] * self.pos_array[:, 0] +
                            self.pos_array[:, 1] * self.pos_array[:, 1] +
                            self.pos_array[:, 2] * self.pos_array[:, 2]) -
                    target) < 0.5 * surface_thickness, opacity_upper,
                opacity_lower)
        # Value iso-surface
        elif button_value == self.cut_options["Value"]:
            return np.where(
                np.abs(self.dslice.values.ravel() - target) <
                0.5 * surface_thickness, opacity_upper, opacity_lower)
        else:
            raise RuntimeError(
                "Unknown cut surface type {}".format(button_value))
