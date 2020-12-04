# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .helpers import PlotArray
from .tools import to_bin_edges, to_bin_centers, make_fake_coord
from .._utils import name_with_unit, value_to_string
from .._scipp import core as sc
import numpy as np


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
                 dim_to_shape=None,
                 dim_label_map=None):

        # The main container of DataArrays
        self.data_arrays = {}
        self.coord_info = {}

        self.axformatter = {}

        axes_dims = list(axes.values())

        # Create dict of DataArrays using information from controller
        for name, array in scipp_obj_dict.items():

            # Store axis tick formatters
            self.axformatter[name] = {}
            self.coord_info[name] = {}
            coord_list = {}

            # Iterate through axes and collect coordinates
            for dim in axes_dims:
                coord, formatter, label, unit = self._axis_coord_and_formatter(
                    dim, array, dim_to_shape[name], dim_label_map)

                self.axformatter[name][dim] = formatter
                self.coord_info[name][dim] = {"label": label, "unit": unit}

                is_histogram = False
                for i, d in enumerate(coord.dims):
                    if d == dim:
                        is_histogram = dim_to_shape[name][
                            d] == coord.shape[i] - 1

                if is_histogram:
                    coord_list[dim] = coord
                else:
                    coord_list[dim] = to_bin_edges(coord, dim)

            # Create a PlotArray helper object that supports slicing where new
            # bin-edge coordinates can be attached to the data
            self.data_arrays[name] = PlotArray(data=array.data,
                                               coords=coord_list)

            # Include masks
            for m in array.masks:
                self.data_arrays[name].masks[m] = array.masks[m]

        # Store dim of multi-dimensional coordinate if present
        self.multid_coord = None
        for array in self.data_arrays.values():
            for dim, coord in array.coords.items():
                if len(coord.dims) > 1:
                    self.multid_coord = dim

        # The main currently displayed data slice
        self.dslice = None
        # Save a copy of the name for simpler access
        self.name = name

    def _axis_coord_and_formatter(self, dim, data_array, dim_to_shape,
                                  dim_label_map):
        """
        Get dimensions from requested axis.
        Also retun axes tick formatters and locators.
        """

        # Create some default axis tick formatter, depending on linear or log
        # scaling.
        formatter = {"linear": None, "log": None, "custom_locator": False}

        coord = None

        if dim not in data_array.coords:
            coord = make_fake_coord(dim, dim_to_shape[dim] + 1)
            return coord, formatter, name_with_unit(var=coord), name_with_unit(
                var=coord, name="")

        tp = data_array.coords[dim].dtype
        coord_info = {}

        if tp == sc.dtype.vector_3_float64:
            coord = make_fake_coord(dim,
                                    dim_to_shape[dim] + 1,
                                    unit=data_array.coords[dim].unit)
            form = lambda val, pos: "(" + ",".join([  # noqa: E731
                value_to_string(item, precision=2)
                for item in data_array.coords[dim].values[int(val)]
            ]) + ")" if (int(val) >= 0 and int(val) < dim_to_shape[dim]
                         ) else ""
            formatter.update({
                "linear": form,
                "log": form,
                "custom_locator": True
            })

        elif tp == sc.dtype.string:
            coord = make_fake_coord(dim,
                                    dim_to_shape[dim] + 1,
                                    unit=data_array.coords[dim].unit)
            form = lambda val, pos: data_array.coords[  # noqa: E731
                dim].values[int(val)] if (int(val) >= 0 and int(val) <
                                          dim_to_shape[dim]) else ""
            formatter.update({
                "linear": form,
                "log": form,
                "custom_locator": True
            })

        elif dim in dim_label_map:
            # non-dimension coordinate
            if dim in data_array.coords:
                coord = data_array.coords[dim]
                coord_values = coord.values
            else:
                coord = make_fake_coord(dim, dim_to_shape[dim] + 1)
                coord_values = coord.values
                if data_array.coords[dim].shape[-1] == dim_to_shape[dim]:
                    coord_values = to_bin_centers(coord, dim).values
            form = lambda val, pos: value_to_string(  # noqa: E731
                data_array.coords[dim_label_map[dim]].values[np.abs(
                    coord_values - val).argmin()])
            formatter.update({"linear": form, "log": form})
            coord_info["label"] = name_with_unit(
                var=data_array.coords[dim_label_map[dim]],
                name=dim_label_map[dim])
            coord_info["unit"] = name_with_unit(
                var=data_array.coords[dim_label_map[dim]], name="")

        else:
            coord = data_array.coords[dim]  # .astype(sc.dtype.float64)

        if len(coord_info) == 0:
            coord_info["label"] = name_with_unit(var=coord)
            coord_info["unit"] = name_with_unit(var=coord, name="")

        return coord, formatter, coord_info["label"], coord_info["unit"]

    def get_axformatter(self, name, dim):
        """
        Get an axformatter for a given data name and dimension.
        """
        return self.axformatter[name][dim]

    def get_slice_coord_bounds(self, name, dim, bounds):
        """
        Return the left, center, and right coordinates for a bin index.
        """
        return self.data_arrays[name].coords[dim][
            dim, bounds[0]].value, self.data_arrays[name].coords[dim][
                dim, bounds[1]].value

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
        return self.data_arrays[name].coords[dim], self.coord_info[name][dim][
            "label"], self.coord_info[name][dim]["unit"]

    def rescale_to_data(self):
        """
        Get the min and max values of the currently displayed slice.
        """
        vmin = None
        vmax = None
        if self.dslice is not None:
            vmin = sc.nanmin(self.dslice.data).value
            vmax = sc.nanmax(self.dslice.data).value
        return vmin, vmax

    def slice_data(self, array, slices):
        """
        Slice the data array according to the dimensions and extents listed
        in slices.
        """
        for dim, [lower, upper] in slices.items():
            # TODO: Could this be optimized for performance?
            if (upper - lower) > 1:
                array = array[dim, lower:upper]
                array.data = sc.rebin(
                    array.data, dim, array.coords[dim],
                    sc.concatenate(array.coords[dim][dim, 0],
                                   array.coords[dim][dim, -1], dim))[dim, 0]
            else:
                array = array[dim, lower]
        return array

    def get_multid_coord(self):
        """
        Return the multi-dimensional coordinate.
        """
        return self.multid_coord
