# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from .._scipp import core as sc
from .. import _utils as su


def instrument_view(scipp_obj=None,
                    dim="tof",
                    bins=None,
                    axes=['tof', 'position'],
                    pixel_size=0.1,
                    **kwargs):
    """
    Plot a 3D view of the instrument, using the 'position` coordinate as the
    detector vector positions.
    Sliders are added to navigate extra dimensions.
    In the case of event data, the dimension given by the `dim` keyword
    argument is histogrammed (with a default number of bins if `bins` is not
    specified). Since this is designed for neutron science, the default
    dimension is neutron time-of-flight ('tof'), but this could be anything
    (wavelength, temperature...)

    Example:

    import scipp.neutron as sn
    sample = sn.load(filename="PG3_4844_event.nxs")
    sn.instrument_view(sample)
    """

    iv = InstrumentView(scipp_obj=scipp_obj,
                        dim=dim,
                        bins=bins,
                        axes=axes,
                        pixel_size=pixel_size,
                        **kwargs)

    return iv.sciplot


class InstrumentView:
    def __init__(self,
                 scipp_obj=None,
                 dim=None,
                 bins=None,
                 axes=None,
                 pixel_size=None,
                 **kwargs):

        from ..plot import plot

        new_bins = bins

        if bins is not None:
            if su.is_dataset(scipp_obj):
                ds = sc.Dataset()
                for name, da in scipp_obj.items():
                    ds[name], new_bins = self._rebin_histogram_data(
                        da, bins, dim)
                scipp_obj = ds
            elif su.is_data_array(scipp_obj):
                scipp_obj, new_bins = self._rebin_histogram_data(
                    scipp_obj, bins, dim)
            else:
                self._raise_input_error()
        else:
            if su.is_dataset(scipp_obj):
                for name, da in scipp_obj.items():
                    new_bins = self._make_default_bins_for_events(
                        da, bins, dim)
            elif su.is_data_array(scipp_obj):
                new_bins = self._make_default_bins_for_events(
                    scipp_obj, bins, dim)
            else:
                self._raise_input_error()

        self.sciplot = plot(scipp_obj,
                            projection="3d",
                            bins=new_bins,
                            axes=axes,
                            pixel_size=pixel_size,
                            **kwargs)
        return

    def _make_default_bins_for_events(self, obj, bins, dim):
        """
        If the DataArray contains events but no bins are specified,
        return a default of 1 bin.
        """
        if sc.contains_events(obj):
            bins = {dim: 1}
        return bins

    def _rebin_histogram_data(self, obj, bins, dim):
        """
        If the DataArray contains histogrammed data and bins are specified,
        then rebin the data.
        """
        if not sc.contains_events(obj):
            # Once data has been histogrammed, we return None as the new bins,
            # since the plot function expects event data if bins are specified.
            return sc.rebin(obj, dim,
                            su.make_bins(data_array=obj, bins=bins,
                                         dim=dim)), None
        else:
            return obj, bins

    def _raise_input_error(self):
        raise RuntimeError("Instrument view only accepts a Dataset "
                           "or a DataArray as an input.")
