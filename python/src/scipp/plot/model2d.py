# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from .. import config
from .model import PlotModel
from .tools import to_bin_centers, mask_to_float, vars_to_err
from .._scipp import core as sc
import numpy as np


class PlotModel2d(PlotModel):
    def __init__(self, *args, resolution=None, **kwargs):

        super().__init__(*args, **kwargs)

        self.button_dims = {}
        self.xyrebin = {}
        self.xywidth = {}
        self.image_pixel_size = {}
        self.vslice = None

        if resolution is not None:
            if isinstance(resolution, int):
                self.image_resolution = {"x": resolution, "y": resolution}
            else:
                self.image_resolution = resolution
        else:
            self.image_resolution = {
                "x": config.plot.width,
                "y": config.plot.height
            }

        return

    def update_axes(self, axparams):

        for xy in "yx":
            # Useful maps
            self.button_dims[xy] = axparams[xy]["dim"]

            # TODO: if labels are used on a 2D coordinates, we need to update
            # the axes tick formatter to use xyrebin coords
            # Create coordinate axes for resampled array to be used as image
            self.xyrebin[xy] = sc.Variable(
                dims=[axparams[xy]["dim"]],
                values=np.linspace(axparams[xy]["lims"][0],
                                   axparams[xy]["lims"][1],
                                   self.image_resolution[xy] + 1),
                unit=self.data_arrays[self.name].coords[axparams[xy]
                                                        ["dim"]].unit)

    def update_data(self, slices, mask_info):
        """
        Slice data according to new slider value and update the image.
        """
        data_slice = self.slice_data(self.data_arrays[self.name], slices)
        # Update pixel widths used for scaling before rebin step
        for xy, dim in self.button_dims.items():
            self.xywidth[xy] = (data_slice.coords[dim][dim, 1:] -
                                data_slice.coords[dim][dim, :-1])
            self.xywidth[xy].unit = sc.units.one

        # Scale by bin width and then rebin in both directions
        # Note that this has to be written as 2 operations to avoid
        # creation of large 2D temporary from broadcast
        #
        # In addition, in the first operation we make a copy to avoid two
        # things:
        #   1. in the case of no dimensions being sliced, we prevent
        #      multiplying into the original data
        #   2. self.vslice needs to be a DataArray and not a DataArrayView for
        #      the rebin step during image resampling, since non-contiguous
        #      data is not accepted by rebin (this happens when an outer dim is
        #      sliced when the slice thickness is zero).
        self.vslice = data_slice * self.xywidth["x"]
        self.vslice *= self.xywidth["y"]

        # Update image with resampling
        new_values = self.update_image(mask_info=mask_info)
        return new_values

    def update_image(self, extent=None, mask_info=None):
        # The order of the dimensions that are rebinned matters if 2D coords
        # are present. We must rebin the base dimension of the 2D coord first.
        xy = "yx"
        if len(self.vslice.coords[self.button_dims["x"]].dims) > 1:
            xy = "xy"

        dimy = self.xyrebin[xy[0]].dims[0]
        dimx = self.xyrebin[xy[1]].dims[0]

        rebin_edges = {dimy: self.xyrebin[xy[0]], dimx: self.xyrebin[xy[1]]}

        resampled_image = self.resample_data(self.vslice,
                                             rebin_edges=rebin_edges)

        # Use Scipp's automatic transpose to match the image x/y axes
        # TODO: once transpose is available for DataArrays,
        # use sc.transpose(dslice, self.button_dims) instead.
        shape = [
            self.xyrebin["y"].shape[0] - 1, self.xyrebin["x"].shape[0] - 1
        ]
        self.dslice = sc.DataArray(coords=rebin_edges,
                                   data=sc.Variable(dims=list(
                                       self.button_dims.values()),
                                                    values=np.ones(shape),
                                                    variances=np.zeros(shape),
                                                    dtype=self.vslice.dtype,
                                                    unit=sc.units.one))

        self.dslice *= resampled_image

        # Update the matplotlib image data
        new_values = {
            "values": self.dslice.values,
            "masks": {},
            "extent": extent
        }

        # Handle masks
        if len(mask_info[self.name]) > 0:
            # Use scipp's automatic broadcast functionality to broadcast
            # lower dimension masks to higher dimensions.
            # TODO: creating a Variable here could become expensive when
            # sliders are being used. We could consider performing the
            # automatic broadcasting once and store it in the Slicer class,
            # but this could create a large memory overhead if the data is
            # large.
            # Here, the data is at most 2D, so having the Variable creation
            # and broadcasting should remain cheap.
            base_mask = sc.Variable(dims=self.dslice.dims,
                                    values=np.ones(self.dslice.shape,
                                                   dtype=np.int32))
            for m in mask_info[self.name]:
                if m in self.dslice.masks:
                    msk = base_mask * sc.Variable(
                        dims=self.dslice.masks[m].dims,
                        values=self.dslice.masks[m].values.astype(np.int32))
                    new_values["masks"][m] = mask_to_float(
                        msk.values, self.dslice.values)
                else:
                    new_values["masks"][m] = None

        return new_values

    def update_viewport(self, xylims, mask_info):

        for xy, dim in self.button_dims.items():
            # Create coordinate axes for resampled image array
            self.xyrebin[xy] = sc.Variable(
                dims=[dim],
                values=np.linspace(xylims[xy][0], xylims[xy][1],
                                   self.image_resolution[xy] + 1),
                unit=self.data_arrays[self.name].coords[dim].unit)
        return self.update_image(extent=np.array(list(
            xylims.values())).flatten(),
                                 mask_info=mask_info)

    def update_profile(self,
                       xdata=None,
                       ydata=None,
                       slices=None,
                       axparams=None,
                       mask_info=None):
        # Find indices of pixel where cursor lies
        dimx = self.xyrebin["x"].dims[0]
        dimy = self.xyrebin["y"].dims[0]
        # Note that xdata and ydata already have the left edge subtracted from
        # them
        ix = int(xdata /
                 (self.xyrebin["x"].values[1] - self.xyrebin["x"].values[0]))
        iy = int(ydata /
                 (self.xyrebin["y"].values[1] - self.xyrebin["y"].values[0]))

        # In the 2D case, we first resample to pixel resolution, to avoid
        # having to potentially resample a very large array in the following
        # slicing step.
        profile_slice = self.resample_data(self.data_arrays[self.name],
                                           rebin_edges={
                                               dimx:
                                               self.xyrebin["x"][dimx,
                                                                 ix:ix + 2],
                                               dimy:
                                               self.xyrebin["y"][dimy,
                                                                 iy:iy + 2]
                                           })[dimx, 0][dimy, 0]

        # Remove the current x and y dims since they will have been manually
        # sliced above
        del slices[dimx]
        del slices[dimy]
        # Slice the remaining dims
        profile_slice = self.slice_data(profile_slice, slices)

        new_values = {self.name: {"values": {}, "variances": {}, "masks": {}}}

        dim = profile_slice.dims[0]
        ydata = profile_slice.values
        xcenters = to_bin_centers(profile_slice.coords[dim], dim).values

        if axparams["x"]["hist"][self.name]:
            new_values[
                self.name]["values"]["x"] = profile_slice.coords[dim].values
            new_values[self.name]["values"]["y"] = np.concatenate(
                (ydata[0:1], ydata))
        else:
            new_values[self.name]["values"]["x"] = xcenters
            new_values[self.name]["values"]["y"] = ydata
        if profile_slice.variances is not None:
            new_values[self.name]["variances"]["x"] = xcenters
            new_values[self.name]["variances"]["y"] = ydata
            new_values[self.name]["variances"]["e"] = vars_to_err(
                profile_slice.variances)

        # Handle masks
        if len(mask_info[self.name]) > 0:
            base_mask = sc.Variable(dims=profile_slice.dims,
                                    values=np.ones(profile_slice.shape,
                                                   dtype=np.int32))
            for m in mask_info[self.name]:
                if m in profile_slice.masks:
                    msk = (base_mask * sc.Variable(
                        dims=profile_slice.masks[m].dims,
                        values=profile_slice.masks[m].values.astype(
                            np.int32))).values
                    if axparams["x"]["hist"][self.name]:
                        msk = np.concatenate((msk[0:1], msk))
                    new_values[self.name]["masks"][m] = mask_to_float(
                        msk, new_values[self.name]["values"]["y"])
                else:
                    new_values[self.name]["masks"][m] = None

        return new_values
