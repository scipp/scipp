# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from .tools import find_limits, to_dict, to_bin_centers
from .. import typing
from ..core import DataArray
from ..core import arange, bins


class Model:
    def __init__(self, data_array, preprocessing_functions):
        self._data_array = data_array
        self._preprocessing_functions = preprocessing_functions

    d


class DataArrayDict(dict):
    """
    Dict of data arrays with matching dimension labels and units. Shape and
    coordinates may mismatch.
    """
    @property
    def dims(self):
        return next(iter(self.values())).dims

    @property
    def sizes(self):
        return next(iter(self.values())).sizes

    @property
    def unit(self):
        da = next(iter(self.values()))
        return da.unit if da.bins is None else da.bins.constituents['data'].unit

    @property
    def meta(self):
        return next(iter(self.values())).meta

    @property
    def data(self):
        return next(iter(self.values())).data


class PlotModel:
    """
    Base class for plot models.

    Upon creation, it:
    - makes a copy of the input data array (including masks)
    - coordinates that contain strings or vectors are converted to fake
        integer coordinates, and axes formatters are updated with lambda
        function formatters.

    The model is where all operations on the data (slicing and resampling) are
    performed.
    """
    def __init__(self, scipp_obj_dict=None):
        self._dims = None
        self._mode = None
        self.data_arrays = {}

        # Create dict of DataArrays using information from controller
        for name, array in scipp_obj_dict.items():
            # TODO for the 3d scatter plot this is problematic: we never
            # touch any of the pos dims, so we don't want to replace coords
            # should model only consider "other" data dims?
            self.data_arrays[name] = array  # self._setup_coords(array)
        self.data_arrays = DataArrayDict(self.data_arrays)

        # Save a copy of the name for simpler access
        # Note this needs to be done before calling update_data_arrays
        self.name = name

        # The main currently displayed data slice
        self.dslice = None

    def update_data(self, slices):
        """
        Slice the data along dimension sliders that are not disabled for all
        entries in the dict of data arrays, and return a dict of 1d value
        arrays for data values, variances, and masks.
        """
        out = DataArrayDict()
        for name, array in self.data_arrays.items():
            out[name] = array
            for dim, sl in slices.items():
                out[name] = out[name][dim, sl]
        return out
