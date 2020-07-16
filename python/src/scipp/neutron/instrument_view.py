# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet

# Scipp imports
from ..plot import plot
from .._scipp import core as sc


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

        if sc.contains_events(scipp_obj) and bins is None:
            bins = {dim: 256}

        self.sciplot = plot(scipp_obj,
                            projection="3d",
                            bins=bins,
                            axes=axes,
                            pixel_size=pixel_size,
                            **kwargs)
        return
