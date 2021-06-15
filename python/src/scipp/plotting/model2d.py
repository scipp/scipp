# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet, Simon Heybrock

from .. import config
from .model import PlotModel
from .resampling_model import resampling_model


class PlotModel2d(PlotModel):
    """
    Model class for 2 dimensional plots.
    """
    def __init__(self, *args, resolution=None, **kwargs):

        self._model = None

        super().__init__(*args, **kwargs)

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
        self._squeeze = []

    def update_data_arrays(self):
        """
        Create or update the internal resampling model.
        """
        super().update_data_arrays()
        if self._model is None:
            self._model = resampling_model(self.data_arrays[self.name])
        else:
            self._model.update_array(self.data_arrays[self.name])

    def update_axes(self, axparams):
        """
        Update axes parameters and coordinate edges for dynamic resampling on
        axis change.
        """
        self.displayed_dims = {}
        for xy in "yx":
            dim = axparams[xy]["dim"]
            self.displayed_dims[xy] = dim
            self._model.resolution[dim] = self.image_resolution[xy]
            self._model.bounds[dim] = None
            # TODO: if labels are used on a 2D coordinates, we need to update
            # the axes tick formatter to use xyrebin coords

    def _update_image(self):
        """
        Resample 2d images to a fixed resolution to handle very large images.
        """
        data = self._model.data
        for dim in self._squeeze:
            data = data[dim, 0]
        self.dslice = data
        return data

    def update_data(self, slices):
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
        return self._update_image()

    def update_viewport(self, xylims):
        """
        When an update to the viewport is requested on a zoom event, set new
        rebin edges and call for a resample of the image.
        """
        for xy, dim in self.displayed_dims.items():
            unit = self.data_arrays[self.name].meta[dim].unit
            self._model.resolution[dim] = self.image_resolution[xy]
            self._model.bounds[dim] = (xylims[xy][0] * unit,
                                       xylims[xy][1] * unit)
        return self._update_image()

    def reset_resampling_model(self):
        self._model.reset()
