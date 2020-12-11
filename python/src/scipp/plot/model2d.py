# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from .. import config
from .model import PlotModel
from .._utils.profile import time
from .resampling_model import resampling_model
from .tools import to_bin_centers, mask_to_float, vars_to_err
from .._scipp import core as sc
import numpy as np


class PlotModel2d(PlotModel):
    """
    Model class for 2 dimensional plots.
    """
    def __init__(self, *args, resolution=None, **kwargs):

        super().__init__(*args, **kwargs)

        self.displayed_dims = {}
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
        self._model = resampling_model(self.data_arrays[self.name])
        self._model.resolution = self.image_resolution  #TODO don't use x and y

    def update_axes(self, axparams):
        """
        Update axes parameters and coordinate edges for dynamic resampling on
        axis change.
        """
        for xy in "yx":
            dim = axparams[xy]["dim"]
            self.displayed_dims[xy] = dim
            unit = self.data_arrays[self.name].coords[dim].unit
            self._model.bounds[dim] = (axparams[xy]["lims"][0] * unit,
                                       axparams[xy]["lims"][1] * unit)
            # TODO: if labels are used on a 2D coordinates, we need to update
            # the axes tick formatter to use xyrebin coords

    def _update(self, extent=None, mask_info=None):
        self.dslice = self._model.data
        values = self.dslice.values
        if self.displayed_dims['x'] == self.dslice.dims[0]:
            values = np.transpose(values)
        new_values = {"values": values, "masks": {}, "extent": extent}
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

    def update_data(self, slices, mask_info):
        """
        Slice the data along dimension sliders that are not disabled for all
        entries in the dict of data arrays.
        Then perform dynamic image resampling based on current viewport.
        """
        print("update_data")
        self.vslice = self.slice_data(self.data_arrays[self.name],
                                      slices,
                                      keep_dims=True)
        # TODO support thick slices
        self._model.bounds['z'] = slices['z'][0]
        return self._update(mask_info=mask_info)
        # Update pixel widths used for scaling before rebin step
        for xy, dim in self.displayed_dims.items():
            self.xywidth[xy] = (self.vslice.coords[dim][dim, 1:] -
                                self.vslice.coords[dim][dim, :-1])
            self.xywidth[xy].unit = sc.units.one

        # Scale by bin width and then rebin in both directions
        # Note that this has to be written as 2 operations to avoid
        # creation of large 2D temporary from broadcast.
        #
        # Note that we only normalize for non-counts data, as rebin already
        # performs the correct resampling for counts.
        # Also note that when normalizing, we perform a copy of the data in
        # the first operation to avoid multiplying the original data in-place.
        #
        # TODO: this can be avoided once we have a generic resample operation
        # that can compute the mean in each new bin and handle any unit.
        if self.vslice.data.unit != sc.units.counts:
            self.vslice.data = self.vslice.data * self.xywidth["x"]
            self.vslice.data *= self.xywidth["y"]
            self.vslice.data.unit = sc.units.one

        # Update image with resampling
        new_values = self.update_image(mask_info=mask_info)
        return new_values

    def _select_bins(self, coord, dim, start, end):
        """
        Method to speed up the finding of bin ranges
        """
        bins = coord.shape[-1]
        if len(coord.dims) != 1:  # TODO find combined min/max
            return dim, slice(0, bins - 1)
        # scipp treats bins as closed on left and open on right: [left, right)
        first = sc.sum(coord <= start, dim).value - 1
        last = bins - sc.sum(coord > end, dim).value
        if first >= last:  # TODO better handling for decreasing
            return dim, slice(0, bins - 1)
        first = max(0, first)
        last = min(bins - 1, last)
        return dim, slice(first, last + 1)

    def resample_data(self, array, rebin_edges):
        """
        Resample a DataArray according to new bin edges.
        """
        # Select bins to speed up rebinning
        for dim in rebin_edges:
            this_slice = self._select_bins(array.coords[dim], dim,
                                           rebin_edges[dim][dim, 0],
                                           rebin_edges[dim][dim, -1])
            array = array[this_slice]

        # Rebin the data
        for dim, edges in rebin_edges.items():
            array.data = sc.rebin(array.data, dim, array.coords[dim], edges)
            for m in array.masks:
                if dim in array.masks[m].dims:
                    array.masks[m] = sc.rebin(array.masks[m], dim,
                                              array.coords[dim], edges)

        # Divide by pixel width if we have normalized in update_data() in the
        # case of non-counts data.
        if array.data.unit != sc.units.counts:
            for dim, edges in rebin_edges.items():
                div = edges[dim, 1:] - edges[dim, :-1]
                div.unit = sc.units.one
                array.data /= div

        return array

    def update_image(self, extent=None, mask_info=None):
        """
        Resample 2d images to a fixed resolution to handle very large images.
        """
        print("update_image")
        # The order of the dimensions that are rebinned matters if 2D coords
        # are present. We must rebin the base dimension of the 2D coord first.
        xy = "yx"
        if len(self.vslice.coords[self.displayed_dims["x"]].dims) > 1:
            xy = "xy"

        dimy = self.xyrebin[xy[0]].dims[0]
        dimx = self.xyrebin[xy[1]].dims[0]

        rebin_edges = {dimy: self.xyrebin[xy[0]], dimx: self.xyrebin[xy[1]]}

        resampled_image = self.resample_data(self.vslice,
                                             rebin_edges=rebin_edges)

        # Slice away the remaining dims because we use slices of range 1 and
        # not 0 in slice_data()
        for dim in set(resampled_image.data.dims) - set(
                list(rebin_edges.keys())):
            resampled_image = resampled_image[dim, 0]

        # Use Scipp's automatic transpose to match the image x/y axes
        # TODO: once transpose is available for DataArrays,
        # use sc.transpose(dslice, self.displayed_dims) instead.
        shape = [
            self.xyrebin["y"].shape[0] - 1, self.xyrebin["x"].shape[0] - 1
        ]
        self.dslice = sc.DataArray(coords=rebin_edges,
                                   data=sc.Variable(dims=list(
                                       self.displayed_dims.values()),
                                                    values=np.ones(shape),
                                                    dtype=sc.dtype.float64,
                                                    unit=sc.units.one))

        if resampled_image.data.variances is not None:
            self.dslice.variances = np.zeros(shape)

        self.dslice *= resampled_image.data
        for m in resampled_image.masks:
            self.dslice.masks[m] = resampled_image.masks[m]

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
        """
        When an update to the viewport is requested on a zoom event, set new
        rebin edges and call for a resample of the image.
        """
        print("update_viewport")
        if self.vslice is None:
            return None

        for xy, dim in self.displayed_dims.items():
            unit = self.data_arrays[self.name].coords[dim].unit
            self._model.bounds[dim] = (xylims[xy][0] * unit,
                                       xylims[xy][1] * unit)
        return self._update(extent=np.array(list(xylims.values())).flatten(), mask_info=mask_info)
        #return self.update_image(extent=np.array(list(
        #    xylims.values())).flatten(),
        #                         mask_info=mask_info)

    @time
    def update_profile(self,
                       xdata=None,
                       ydata=None,
                       slices=None,
                       axparams=None,
                       profile_dim=None,
                       mask_info=None):
        """
        Slice down all dimensions apart from the profile dimension, and send
        the data values, variances and masks back to the `PlotController`.

        TODO: remove duplicate code between this and update_profile in model1d.
        """

        # Find indices of pixel where cursor lies
        dimx = self.displayed_dims['x']
        dimy = self.displayed_dims['y']
        if len(slices) == 1:
            dim, [start, stop] = slices[0]
            self._profile_model = resampling_model(
                self.data_arrays[self.name][dim, start:stop])
        else:
            self._profile_model = resampling_model(self.data_arrays[self.name])
        self._profile_model.resolution = {'x': 1, 'y': 1, profile_dim: 200}
        x = self._model.edges[1]
        y = self._model.edges[0]
        # Note that xdata and ydata already have the left edge subtracted from
        # them
        ix = int(xdata / (x.values[1] - x.values[0]))
        iy = int(ydata / (y.values[1] - y.values[0]))
        unit = self.data_arrays[self.name].coords[profile_dim].unit
        self._profile_model.bounds = {
            dimx: (x[dimx, ix], x[dimx, ix + 1]),
            dimy: (y[dimy, iy], y[dimy, iy + 1]),
            profile_dim: None
        }
        profile_slice = self._profile_model.data[dimx, 0][dimy, 0]

        # Slice the remaining dims
        new_values = {self.name: {"values": {}, "variances": {}, "masks": {}}}

        ydata = profile_slice.data.values
        xcenters = to_bin_centers(profile_slice.coords[profile_dim],
                                  profile_dim).values

        if axparams["x"]["hist"][self.name]:
            new_values[self.name]["values"]["x"] = profile_slice.coords[
                profile_dim].values
            new_values[self.name]["values"]["y"] = np.concatenate(
                (ydata[0:1], ydata))
        else:
            new_values[self.name]["values"]["x"] = xcenters
            new_values[self.name]["values"]["y"] = ydata
        if profile_slice.data.variances is not None:
            new_values[self.name]["variances"]["x"] = xcenters
            new_values[self.name]["variances"]["y"] = ydata
            new_values[self.name]["variances"]["e"] = vars_to_err(
                profile_slice.data.variances)
        return new_values

        # Handle masks
        if len(mask_info[self.name]) > 0:
            base_mask = sc.Variable(dims=profile_slice.data.dims,
                                    values=np.ones(profile_slice.data.shape,
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
