# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .._scipp import core as sc


class PlotModel:
    def __init__(self, controller=None, scipp_obj_dict=None):

        # The main container of DataArrays
        self.data_arrays = {}

        # Create dict of DataArrays using information from controller
        for name, array in scipp_obj_dict.items():

            # Create a DataArray with units of counts, and bin-edge
            # coordinates, because it is to be passed to rebin during the
            # resampling stage.
            self.data_arrays[name] = sc.DataArray(data=sc.Variable(
                dims=list(controller.dim_to_shape[name].keys()),
                unit=sc.units.counts,
                values=array.values,
                variances=array.variances,
                dtype=sc.dtype.float64))

            # Add bin-edge coordinates, which are either dimension or non-
            # dimension coordinates
            for dim, coord in controller.coords[name].items():
                self.data_arrays[name].coords[dim] = coord

            # Include masks
            for n, msk in array.masks.items():
                self.data_arrays[name].masks[n] = msk

        # The main currently displayed data slice
        self.dslice = None
        # Save a copy of the name for simpler access
        self.name = controller.name

    def _select_bins(self, coord, dim, start, end):
        """
        Method to speed up the finding of bin ranges
        """
        bins = coord.shape[-1]
        if len(coord.dims) != 1:  # TODO find combined min/max
            return dim, slice(0, bins - 1)
        # scipp treats bins as closed on left and open on right: [left, right)
        first = sc.sum(coord <= start, dim).value - 1
        last = bins - sc.sum(coord > end, dim).value
        if first >= last:  # TODO better handling for decreasing
            return dim, slice(0, bins - 1)
        first = max(0, first)
        last = min(bins - 1, last)
        return dim, slice(first, last + 1)

    def resample_data(self, array, rebin_edges):
        """
        Resample a DataArray according to new bin edges.
        """
        dslice = array
        # Select bins to speed up rebinning
        for dim in rebin_edges:
            this_slice = self._select_bins(array.coords[dim], dim,
                                           rebin_edges[dim][dim, 0],
                                           rebin_edges[dim][dim, -1])
            dslice = dslice[this_slice]

        # Rebin the data
        for dim, edges in rebin_edges.items():
            dslice = sc.rebin(dslice, dim, edges)

        # Divide by pixel width
        # TODO: can this loop be combined with the one above?
        for dim, edges in rebin_edges.items():
            div = edges[dim, 1:] - edges[dim, :-1]
            div.unit = sc.units.one
            dslice /= div

        return dslice

    def rescale_to_data(self):
        """
        Get the min and max values of the currently displayed slice.
        """
        vmin = None
        vmax = None
        if self.dslice is not None:
            vmin = sc.min(self.dslice.data).value
            vmax = sc.max(self.dslice.data).value
        return vmin, vmax
