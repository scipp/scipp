# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .tools import find_limits, to_dict, to_bin_centers
from .. import typing
from ..core import DataArray
from ..core import arange, bins
from .resampling_model import ResamplingMode


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
            self.data_arrays[name] = self._setup_coords(array)
        self.data_arrays = DataArrayDict(self.data_arrays)

        # Save a copy of the name for simpler access
        # Note this needs to be done before calling update_data_arrays
        self.name = name

        self.update()

        # The main currently displayed data slice
        self.dslice = None

    def _dims_updated(self):
        pass

    @property
    def unit(self):
        return self.data_arrays.unit

    @property
    def dims(self):
        return self._dims

    @dims.setter
    def dims(self, dims):
        self._dims = dims
        self._dims_updated()

    def _mode_updated(self):
        pass

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, m: ResamplingMode):
        self._mode = m
        self._mode_updated()

    def _setup_coords(self, array):
        data = array.data
        if array.bins is not None:
            data = self._setup_event_coords(data)
        coord_list = {dim: self._axis_coord(array, dim) for dim in array.dims}
        return DataArray(data=data, coords=coord_list, masks=to_dict(array.masks))

    def _setup_event_coords(self, data):
        tmp = data.bins.constituents
        coord_list = {}
        array = tmp['data']
        for dim in data.dims:
            # Drop any coords not required for resampling to save memory and compute
            if dim in array.meta:
                coord = array.meta[dim]
                if typing.has_datetime_type(coord):
                    coord = coord - coord.min()
                coord_list[dim] = coord
        tmp['data'] = DataArray(data=array.data,
                                coords=coord_list,
                                masks=to_dict(array.masks))
        return bins(**tmp)

    def _axis_coord(self, array, dim):
        """
        Get coord for requested dim.
        """
        if dim in array.meta:
            coord = array.meta[dim]
            if typing.has_vector_type(coord) or typing.has_string_type(coord):
                coord = arange(dim=dim, start=0, stop=array.sizes[dim])
            elif typing.has_datetime_type(coord):
                coord = coord - coord.min()
            # TODO This hack looks "wrong". This may be an indicator that resampling
            # may not be the correct default choice.
            # If there is a bin-edge coord but no corresponding event-coord then `bin`
            # cannot handle this. We could first bin without this and then use `rebin`.
            if coord.sizes[dim] != array.sizes[dim]:
                if array.bins is not None and dim not in array.bins.coords:
                    coord = to_bin_centers(coord, dim)
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
