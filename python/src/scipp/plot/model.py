# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .tools import to_bin_edges, to_bin_centers, make_fake_coord
from .._utils import value_to_string
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

        self.axformatter = {}

        axes_dims = list(axes.values())

        # Create dict of DataArrays using information from controller
        for name, array in scipp_obj_dict.items():

            # Store axis tick formatters
            self.axformatter[name] = {}

            # Create a DataArray with units of counts, and bin-edge
            # coordinates, because it is to be passed to rebin during the
            # resampling stage.
            self.data_arrays[name] = sc.DataArray(
                data=sc.Variable(dims=list(dim_to_shape[name].keys()),
                                 unit=sc.units.counts,
                                 values=array.values,
                                 variances=array.variances,
                                 dtype=sc.dtype.float64))

            # Iterate through axes and collect coordinates
            for dim in axes_dims:
                coord, formatter = self._axis_coord_and_formatter(
                    dim, array, dim_to_shape[name], dim_label_map)

                self.axformatter[name][dim] = formatter

                is_histogram = None
                for i, d in enumerate(coord.dims):
                    if d == dim:
                        is_histogram = dim_to_shape[name][
                            d] == coord.shape[i] - 1

                if is_histogram:
                    self.data_arrays[name].coords[dim] = coord
                else:
                    self.data_arrays[name].coords[dim] = to_bin_edges(
                        coord, dim)

            # Include masks
            for m, msk in array.masks.items():
                mask_dims = msk.dims
                for dim in mask_dims:
                    if dim not in axes_dims:
                        mask_dims[mask_dims.index(dim)] = dim_label_map[dim]
                self.data_arrays[name].masks[m] = sc.Variable(
                    dims=mask_dims, values=msk.values, dtype=msk.dtype)

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

        if dim in data_array.coords:

            underlying_dim = data_array.coords[dim].dims[-1]
            tp = data_array.coords[dim].dtype

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

            elif dim != underlying_dim:
                # non-dimension coordinate
                if underlying_dim in data_array.coords:
                    coord = data_array.coords[underlying_dim]
                    coord = sc.Variable([dim],
                                        values=coord.values,
                                        variances=coord.variances,
                                        unit=coord.unit,
                                        dtype=sc.dtype.float64)
                    coord_values = coord.values
                else:
                    coord = make_fake_coord(dim, dim_to_shape[dim] + 1)
                    coord_values = coord.values
                    if data_array.coords[dim].shape[-1] == dim_to_shape[dim]:
                        coord_values = to_bin_centers(coord, dim).values
                form = lambda val, pos: value_to_string(  # noqa: E731
                    data_array.coords[dim].values[np.abs(coord_values - val).
                                                  argmin()])
                formatter.update({"linear": form, "log": form})

            else:
                coord = data_array.coords[dim].astype(sc.dtype.float64)

        else:
            # dim not found in data_array.coords
            coord = make_fake_coord(dim, dim_to_shape[dim] + 1)

        return coord, formatter

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
        return self.data_arrays[name].coords[dim]

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
            array = array[dim, lower:upper]
            if (upper - lower) > 1:
                array = sc.rebin(
                    array, dim,
                    sc.concatenate(array.coords[dim][dim, 0],
                                   array.coords[dim][dim, -1], dim))[dim, 0]
            else:
                array = array[dim, 0]

        return array
