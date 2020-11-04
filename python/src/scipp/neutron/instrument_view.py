# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

from .._scipp import core as sc
from .. import _utils as su


def _make_default_bins_for_events(obj, bins):
    """
    If the DataArray contains events but no bins are specified,
    return a default of 1 bin.
    """
    return bins


def _rebin_histogram_data(obj, bins):
    """
    If the DataArray contains histogrammed data and bins are specified,
    then rebin the data.
    """
    new_bins = bins
    if not isinstance(bins, dict):
        new_bins = {"tof": bins}
    for dim, val in new_bins.items():
        obj = sc.rebin(obj, dim, su.make_bins(data_array=obj,
                                              bins=val,
                                              dim=dim))
    # Once data has been histogrammed, we return None as the new bins,
    # since the plot function expects event data if bins are specified.
    return obj, None


def _raise_input_error():
    raise RuntimeError("Instrument view only accepts a Dataset "
                       "or a DataArray as an input.")


def instrument_view(scipp_obj=None,
                    bins=None,
                    positions="position",
                    pixel_size=0.1,
                    **kwargs):
    """
    Plot a 3D view of the instrument, using the `position` coordinate as the
    detector vector positions.
    Use the `positions` argument to change the vectors used as pixel positions.
    Sliders are added to navigate extra dimensions.
    In the case of event data, histogramming is performed according to the
    `bins` keyword argument. `bins` can be a dict of
    {dimension: nbins OR ndarray}, or directly the underlying nbins OR ndarray.
    Since this is designed for neutron science, the default dimension is
    neutron time-of-flight ('tof'), if bins is not a dict.

    Example:

    import scipp.neutron as sn
    sample = sn.load(filename="PG3_4844_event.nxs")
    sn.instrument_view(sample, bins=256)

    The logic for the bins control is the following:
    1. If the input data contains events, and bins is None, then a default of
       1 bin along the "tof" dimension is generated.
    2. If the input data is histogrammed, and bins is None, bins remains None.
    3. If bins is not None, and bins is not a dict, then bins becomes
       {"tof": bins}.
    4. If bins is not None, and the input data is histogrammed, each dimension
       in the bins.keys() of the input data is rebinned according to the
       corresponding bins.values(). bins is then forwarded as None to plot_3d.
    """

    from ..plot import plot

    new_bins = bins

    if bins is not None:
        if su.is_dataset(scipp_obj):
            ds = sc.Dataset()
            for name, da in scipp_obj.items():
                ds[name], new_bins = _rebin_histogram_data(da, bins)
            scipp_obj = ds
        elif su.is_data_array(scipp_obj):
            scipp_obj, new_bins = _rebin_histogram_data(scipp_obj, bins)
        else:
            _raise_input_error()
    else:
        if su.is_dataset(scipp_obj):
            for name, da in scipp_obj.items():
                new_bins = _make_default_bins_for_events(da, bins)
        elif su.is_data_array(scipp_obj):
            new_bins = _make_default_bins_for_events(scipp_obj, bins)
        else:
            _raise_input_error()

    return plot(scipp_obj,
                projection="3d",
                bins=new_bins,
                positions=positions,
                pixel_size=pixel_size,
                **kwargs)
