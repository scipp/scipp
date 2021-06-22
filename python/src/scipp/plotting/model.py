# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .plot import DataArrayDict
from .tools import find_limits, to_dict
from ..utils import vector_type, string_type, datetime_type
from .._scipp import core as sc
from .._variable import arange


class PlotModel:
    """
    Base class for `model`.

    Upon creation, it:
    - makes a copy of the input data array (including masks)
    - units of the original data are saved, and units of the copy are set to
        counts, because `rebin` is used for resampling and only accepts counts.
    - it replaces all coordinates with corresponding bin-edges, which allows
        for much more generic plotting code
    - coordinates that contain strings or vectors are converted to fake
        integer coordinates, and axes formatters are updated with lambda
        function formatters.

    The model is where all operations on the data (slicing and resampling) are
    performed.
    """
    def __init__(self, scipp_obj_dict=None, name=None):
        self._dims = None
        self.data_arrays = {}

        # Create dict of DataArrays using information from controller
        for name, array in scipp_obj_dict.items():
            coord_list = {
                dim: self._axis_coord(array, dim)
                for dim in array.dims
            }
            self.data_arrays[name] = sc.DataArray(data=array.data,
                                                  coords=coord_list,
                                                  masks=to_dict(array.masks))
        self._data_arrays = DataArrayDict(self.data_arrays)

        # Save a copy of the name for simpler access
        # Note this needs to be done before calling update_data_arrays
        self.name = name

        self.update()

        # The main currently displayed data slice
        self.dslice = None

    def _dims_updated(self):
        pass

    @property
    def dims(self):
        return self._dims

    @dims.setter
    def dims(self, dims):
        self._dims = dims
        self._dims_updated()

    def _axis_coord(self, array, dim):
        """
        Get coord for requested dim.
        """
        if dim in array.meta:
            coord = array.meta[dim]
            if vector_type(coord) or string_type(coord):
                coord = arange(dim=dim, start=0, stop=array.sizes[dim])
            elif datetime_type(coord):
                coord = coord - sc.min(coord)
        else:
            coord = arange(dim=dim, start=0, stop=array.sizes[dim])
        return coord

    def rescale_to_data(self, scale=None):
        """
        Get the min and max values of the currently displayed slice.
        """
        if self.dslice is not None:
            return find_limits(self.dslice.data, scale=scale)[scale]
        else:
            return [None, None]

    # TODO remove once model3d is refactored
    def slice_data(self, array, slices):
        """
        Slice the data array according to the dimensions and extents listed
        in slices.
        """
        for dim, [lower, upper] in slices.items():
            # TODO: Could this be optimized for performance?
            # Note: we use the range 1 [dim, i:i+1] slicing here instead of
            # index slicing [dim, i] so that we hit the correct branch in
            # rebin, because in the case of slicing an outer dim, rebin-inner
            # cannot deal with non-continuous data as an input.
            array = array[dim, lower:upper]
            if (upper - lower) > 1:
                array.data = sc.rebin(
                    array.data, dim, array.meta[dim],
                    sc.concatenate(array.meta[dim][dim, 0],
                                   array.meta[dim][dim, -1], dim))
            array = array[dim, 0]
        return array
