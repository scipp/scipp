# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .model import PlotModel
from .tools import find_limits
from .resampling_model import resampling_model
import numpy as np


class PlotModel1d(PlotModel):
    """
    Model class for 1 dimensional plots.
    """
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        for name, array in self.data_arrays.items():
            if array.bins is not None:
                raise RuntimeError(
                    "1-D plots of binned data not implemented. Histogram "
                    "manually, e.g., using `plot(array.bins.sum())`")
        self.dim = None

    def update_axes(self, axparams):
        """
        Update axes parameters on axis change.
        """
        self.dim = axparams["x"]["dim"]

    def update_data(self, slices):
        """
        Slice the data along dimension sliders that are not disabled for all
        entries in the dict of data arrays, and return a dict of 1d value
        arrays for data values, variances, and masks.
        """
        self.dslice = {}
        out = {}
        for name in self.data_arrays:
            squeeze = []
            model = resampling_model(self.data_arrays[name])
            # TODO use same slices API as in Resampling model so we can just
            # pass it directly
            for dim, bounds in slices.items():
                if isinstance(bounds, int):
                    model.bounds[dim] = bounds
                else:
                    start, stop = bounds
                    if start + 1 == stop:
                        model.bounds[dim] = start
                    else:
                        squeeze.append(dim)
                        model.resolution[dim] = 1
                        model.bounds[dim] = bounds
            data = model.data
            for dim in squeeze:
                data = data[dim, 0]
            self.dslice[name] = data
            out[name] = data
        return out

    def rescale_to_data(self, scale=None):
        """
        Get the min and max values of the currently displayed slice.
        """
        if self.dslice is not None:
            limits = np.array([
                find_limits(array.data, scale=scale)[scale]
                for array in self.dslice.values()
            ])
            return [np.amin(limits[:, 0]), np.amax(limits[:, 1])]
        else:
            return [None, None]
