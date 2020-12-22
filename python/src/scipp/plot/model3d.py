# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .model import PlotModel
from .tools import to_bin_centers
from .._scipp import core as sc
import numpy as np


class PlotModel3d(PlotModel):
    """
    Model class for 3 dimensional plots.

    It handles updating the data values when using dimension sliders as well as
    updating opacities for the cut surfaces.
    """
    def __init__(self, *args, scipp_obj_dict=None, positions=None, **kwargs):

        super().__init__(*args, scipp_obj_dict=scipp_obj_dict, **kwargs)

        self.displayed_dims = {}
        self.positions = positions
        self.pos_array = None
        self.pos_coord = None
        self.pos_unit = None
        self.cut_options = None

        # If positions are specified, then the x, y, z points positions can
        # never change
        if self.positions is not None:
            self.pos_coord = scipp_obj_dict[self.name].meta[self.positions]
            self.pos_array = np.array(self.pos_coord.values, dtype=np.float32)

    def initialise(self, cut_options):
        """
        The model handles calculations of opacities for the cut surface, so it
        needs to know which are the possible cut surface options. Those are set
        once the `PlotPanel3d` has been created.
        """
        self.cut_options = cut_options

    def update_axes(self, axparams):
        """
        When axes are changed, a new meshgrid of positions is computed.
        """

        # If no positions are supplied, create a meshgrid from coordinates
        if self.positions is None:

            for xyz in "zyx":
                self.displayed_dims[xyz] = axparams[xyz]["dim"]

            z, y, x = np.meshgrid(
                to_bin_centers(
                    self.data_arrays[self.name].meta[axparams['z']["dim"]],
                    axparams["z"]["dim"]).values,
                to_bin_centers(
                    self.data_arrays[self.name].meta[axparams['y']["dim"]],
                    axparams["y"]["dim"]).values,
                to_bin_centers(
                    self.data_arrays[self.name].meta[axparams['x']["dim"]],
                    axparams["x"]["dim"]).values,
                indexing='ij')

            self.pos_array = np.array(
                [x.ravel(), y.ravel(), z.ravel()], dtype=np.float32).T

        return {"positions": self.pos_array}

    def get_slice_values(self, mask_info):
        """
        Get data and mask values as numpy arrays.
        """
        new_values = {
            "values": self.dslice.data.values.astype(np.float32).ravel(),
            # "masks": None
        }

        # Handle masks
        # msk = None
        if len(mask_info[self.name]) > 0:
            # Use automatic broadcasting in Scipp variables
            msk = sc.Variable(dims=self.dslice.data.dims,
                              values=np.zeros(self.dslice.data.shape,
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
        data_slice = self.slice_data(self.data_arrays[self.name], slices)

        # Use automatic broadcast if positions are not used
        if self.positions is None:
            shape = [
                data_slice.meta[self.displayed_dims["z"]].shape[0] - 1,
                data_slice.meta[self.displayed_dims["y"]].shape[0] - 1,
                data_slice.meta[self.displayed_dims["x"]].shape[0] - 1
            ]

            self.dslice = sc.DataArray(
                coords={
                    self.displayed_dims["z"]:
                    data_slice.meta[self.displayed_dims["z"]],
                    self.displayed_dims["y"]:
                    data_slice.meta[self.displayed_dims["y"]],
                    self.displayed_dims["x"]:
                    data_slice.meta[self.displayed_dims["x"]]
                },
                data=sc.Variable(dims=[
                    self.displayed_dims["z"], self.displayed_dims["y"],
                    self.displayed_dims["x"]
                ],
                                 values=np.ones(shape),
                                 variances=np.zeros(shape),
                                 dtype=data_slice.data.dtype,
                                 unit=sc.units.one),
                masks=data_slice.masks)

            self.dslice *= data_slice.data
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

    def get_positions_extents(self, pixel_size):
        extents = {}
        for xyz in "xyz":
            x = getattr(sc.geometry, xyz)(self.pos_coord)
            extents[xyz] = {
                "lims": [
                    sc.min(x).value - 0.5 * pixel_size,
                    sc.max(x).value + 0.5 * pixel_size
                ],
                "unit":
                self.pos_coord.unit
            }
        return extents
