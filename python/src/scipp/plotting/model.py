# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .formatters import VectorFormatter, StringFormatter, \
                        DateFormatter, LabelFormatter
from .tools import to_bin_edges, to_bin_centers, make_fake_coord, \
                   find_limits, to_dict
from ..utils import name_with_unit, vector_type, string_type, datetime_type
from .._scipp import core as sc
import enum


class Kind(enum.Enum):
    """
    Small enum listing the special cases for the axis tick formatters.
    """
    vector = enum.auto()
    string = enum.auto()
    datetime = enum.auto()
    other = enum.auto()


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
    def __init__(self,
                 scipp_obj_dict=None,
                 name=None,
                 axes=None,
                 dim_label_map=None):

        self.interface = {}

        # The main container of DataArrays
        self.data_arrays = {}
        self.backup = {}
        self.coord_info = {}
        self.axformatter = {}

        # maps, e.g., x,y,0 -> tof,spectrum,temperature
        axes_dims = list(axes.values())

        # Create dict of DataArrays using information from controller
        for name, array in scipp_obj_dict.items():

            # Store axis tick formatters
            self.axformatter[name] = {}
            self.coord_info[name] = {}
            coord_list = {}

            # Iterate through axes and collect coordinates
            for dim in axes_dims:

                coord, formatter, label, unit, offset = \
                    self._axis_coord_and_formatter(
                        dim, array, dim_label_map)

                self.axformatter[name][dim] = formatter
                self.coord_info[name][dim] = {"label": label, "unit": unit}

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

    def _axis_coord_and_formatter(self, dim, data_array, dim_label_map):
        """
        Get dimensions from requested axis.
        Also return axes tick formatters and locators.
        """

        # Create some default axis tick formatter, depending on linear or log
        # scaling.
        formatter = {"linear": None, "log": None, "custom_locator": False}

        kind = {}
        offset = 0.0
        coord_label = None
        form = None

        has_no_coord = dim not in data_array.meta
        kind[dim] = Kind.other
        keys = []
        if not has_no_coord:
            keys.append(dim)
        if dim in dim_label_map:
            keys.append(dim_label_map[dim])

        for key in keys:
            if has_vector_type(data_array.meta[key]):
                kind[key] = Kind.vector
            elif has_string_type(data_array.meta[key]):
                kind[key] = Kind.string
            elif has_datetime_type(data_array.meta[key]):
                kind[key] = Kind.datetime
            else:
                kind[key] = Kind.other

        # Get the coordinate from the DataArray or generate a fake one
        if has_no_coord or (kind[dim] == Kind.vector) or (kind[dim]
                                                          == Kind.string):
            coord = make_fake_coord(dim, data_array.sizes[dim] + 1)
            if not has_no_coord:
                coord.unit = data_array.meta[dim].unit
        elif kind[dim] == Kind.datetime:
            coord = data_array.meta[dim]
            offset = sc.min(coord)
            coord = coord - offset
        else:
            coord = data_array.meta[dim]
        # Convert the coordinate to float because rebin (used in 2d plots) does
        # not currently support integer coordinates
        if (coord.dtype != sc.dtype.float32) and (coord.dtype !=
                                                  sc.dtype.float64):
            coord = coord.astype(sc.dtype.float64)

        # Set up tick formatters
        if dim in dim_label_map:
            key = dim_label_map[dim]
        else:
            key = dim

        if kind[key] == Kind.vector:
            form = VectorFormatter(data_array.meta[key].values,
                                   data_array.sizes[dim]).formatter
            formatter["custom_locator"] = True
        elif kind[key] == Kind.string:
            form = StringFormatter(data_array.meta[key].values,
                                   data_array.sizes[dim]).formatter
            formatter["custom_locator"] = True
        elif kind[key] == Kind.datetime:
            form = DateFormatter(offset, key, self.interface).formatter
        elif dim in dim_label_map:
            coord_values = coord.values
            if has_no_coord:
                # In this case we always have a bin-edge coord
                coord_values = to_bin_centers(coord, dim).values
            else:
                if data_array.meta[dim].shape[-1] == data_array.sizes[dim]:
                    coord_values = to_bin_centers(coord, dim).values
            form = LabelFormatter(data_array.meta[dim_label_map[dim]].values,
                                  coord_values).formatter

        if form is not None:
            formatter.update({"linear": form, "log": form})

        if dim in dim_label_map:
            coord_label = name_with_unit(
                var=data_array.meta[dim_label_map[dim]],
                name=dim_label_map[dim])
            coord_unit = name_with_unit(
                var=data_array.meta[dim_label_map[dim]], name="")
        else:
            coord_label = name_with_unit(var=coord)
            coord_unit = name_with_unit(var=coord, name="")

        return coord, formatter, coord_label, coord_unit, offset

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
        return self.data_arrays[name].meta[dim], self.coord_info[name][dim][
            "label"], self.coord_info[name][dim]["unit"]

    def rescale_to_data(self, scale=None):
        """
        Get the min and max values of the currently displayed slice.
        """
        if self.dslice is not None:
            return find_limits(self.dslice.data, scale=scale)[scale]
        else:
            return [None, None]

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
