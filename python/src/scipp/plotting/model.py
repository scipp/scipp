# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .tools import to_bin_edges, make_fake_coord, find_limits, to_dict
from ..utils import vector_type, string_type, datetime_type
from .._scipp import core as sc


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

        self.interface = {}

        # The main container of DataArrays
        self.data_arrays = {}
        self.backup = {}

        # Create dict of DataArrays using information from controller
        for name, array in scipp_obj_dict.items():
            coord_list = {}

            # Iterate through axes and collect coordinates
            for dim in array.dims:
                coord = self._axis_coord(array, dim)

                is_histogram = False
                for i, d in enumerate(coord.dims):
                    if d == dim:
                        is_histogram = array.sizes[d] == coord.shape[i] - 1

                if is_histogram:
                    coord_list[dim] = coord
                else:
                    coord_list[dim] = to_bin_edges(coord, dim)

            self.backup.update({name: {"array": array, "coords": coord_list}})

        # Save a copy of the name for simpler access
        # Note this needs to be done before calling update_data_arrays
        self.name = name

        # Update the internal dict of arrays from the original input
        self.update_data_arrays()

        # Store dim of multi-dimensional coordinate if present
        self.multid_coord = None
        for array in self.data_arrays.values():
            for dim, coord in array.meta.items():
                if len(coord.dims) > 1:
                    self.multid_coord = dim

        # The main currently displayed data slice
        self.dslice = None

    def _axis_coord(self, array, dim):
        """
        Get coord for requested dim.
        """
        if dim in array.meta:
            coord = array.meta[dim]
            if vector_type(coord) or string_type(coord):
                coord = make_fake_coord(dim, array.sizes[dim])
            elif datetime_type(coord):
                coord = coord - sc.min(coord)
        else:
            coord = make_fake_coord(dim, array.sizes[dim])
        # Convert the coordinate to float because rebin (used in 2d plots) does
        # not currently support integer coordinates
        if coord.dtype in [sc.dtype.float32, sc.dtype.float64]:
            coord = coord.astype(sc.dtype.float64)
        return coord

    def update_data_arrays(self):
        for name, item in self.backup.items():
            self.data_arrays[name] = sc.DataArray(data=item["array"].data,
                                                  coords=item["coords"],
                                                  masks=to_dict(
                                                      item["array"].masks))

    def get_axformatter(self, name, dim):
        """
        Get an axformatter for a given data name and dimension.
        """
        return self.axformatter[name][dim]

    def get_slice_coord_bounds(self, name, dim, bounds):
        """
        Return the left, center, and right coordinates for a bin index.
        """
        return self.data_arrays[name].meta[dim][
            dim,
            bounds[0]].value, self.data_arrays[name].meta[dim][dim,
                                                               bounds[1]].value

    def get_data_names(self):
        """
        List all names in dict of data arrays.
        This is usually only a single name, but can be more than one for 1d
        plots.
        """
        return list(self.data_arrays.keys())

    def get_data_coord(self, name, dim):
        """
        Get a coordinate along a requested dimension.
        """
        return self.data_arrays[name].meta[dim]

    def rescale_to_data(self, scale=None):
        """
        Get the min and max values of the currently displayed slice.
        """
        if self.dslice is not None:
            return find_limits(self.dslice.data, scale=scale)[scale]
        else:
            return [None, None]

    # TODO remove ince model3d is refactored
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

    def get_multid_coord(self):
        """
        Return the multi-dimensional coordinate.
        """
        return self.multid_coord

    def update_profile_model(self, *args, **kwargs):
        return

    def connect(self, callbacks):
        for name, func in callbacks.items():
            self.interface[name] = func
