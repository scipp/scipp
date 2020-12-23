# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet, Simon Heybrock

from .. import config
from .model import PlotModel
from .resampling_model import resampling_model
from .._scipp import core as sc
import numpy as np


class PlotModel2d(PlotModel):
    """
    Model class for 2 dimensional plots.
    """
    def __init__(self, *args, resolution=None, **kwargs):

        super().__init__(*args, **kwargs)

        self.displayed_dims = {}

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
        self._squeeze = []

    def update_axes(self, axparams):
        """
        Update axes parameters and coordinate edges for dynamic resampling on
        axis change.
        """
        for xy in "yx":
            dim = axparams[xy]["dim"]
            self.displayed_dims[xy] = dim
            unit = self.data_arrays[self.name].meta[dim].unit
            self._model.resolution[dim] = self.image_resolution[xy]
            self._model.bounds[dim] = (axparams[xy]["lims"][0] * unit,
                                       axparams[xy]["lims"][1] * unit)
            # TODO: if labels are used on a 2D coordinates, we need to update
            # the axes tick formatter to use xyrebin coords

    def get_slice_values(self, mask_info, extent=None):
        values = self.dslice.values
        transpose = self.displayed_dims['x'] == self.dslice.dims[0]
        if transpose:
            values = np.transpose(values)
        slice_values = {"values": values, "extent": extent}
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
            if transpose:
                slice_values["masks"] = np.transpose(msk.values)
            else:
                slice_values["masks"] = msk.values
        return slice_values

    def _update_image(self, extent=None, mask_info=None):
        """
        Resample 2d images to a fixed resolution to handle very large images.
        """
        data = self._model.data
        for dim in self._squeeze:
            data = data[dim, 0]
        self.dslice = data
        return self.get_slice_values(mask_info=mask_info, extent=extent)

    def update_data(self, slices, mask_info):
        """
        Slice the data along dimension sliders that are not disabled for all
        entries in the dict of data arrays.
        Then perform dynamic image resampling based on current viewport.
        """
        self._squeeze = []
        for dim, [start, stop] in slices.items():
            if start + 1 == stop:
                self._model.bounds[dim] = start
            else:
                self._squeeze.append(dim)
                self._model.resolution[dim] = 1
                self._model.bounds[dim] = (start, stop)
        return self._update_image(mask_info=mask_info)

    def update_viewport(self, xylims, mask_info):
        """
        When an update to the viewport is requested on a zoom event, set new
        rebin edges and call for a resample of the image.
        """
        for xy, dim in self.displayed_dims.items():
            unit = self.data_arrays[self.name].meta[dim].unit
            self._model.resolution[dim] = self.image_resolution[xy]
            self._model.bounds[dim] = (xylims[xy][0] * unit,
                                       xylims[xy][1] * unit)
        return self._update_image(extent=np.array(list(
            xylims.values())).flatten(),
                                  mask_info=mask_info)

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
        self._profile_model.resolution = {dimx: 1, dimy: 1, profile_dim: 200}
        x = self._model.data.meta[dimx]
        y = self._model.data.meta[dimy]
        # Note that xdata and ydata already have the left edge subtracted from
        # them
        ix = int(xdata / (x.values[1] - x.values[0]))
        iy = int(ydata / (y.values[1] - y.values[0]))
        self._profile_model.bounds = {
            dimx: (x[dimx, ix], x[dimx, ix + 1]),
            dimy: (y[dimy, iy], y[dimy, iy + 1]),
            profile_dim: None
        }
        return {
            self.name:
            self._make_profile(self._profile_model.data[dimx, 0][dimy, 0],
                               profile_dim, mask_info[self.name])
        }
