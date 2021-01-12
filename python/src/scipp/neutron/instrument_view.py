# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Neil Vaytet


def instrument_view(scipp_obj=None,
                    positions="position",
                    aspect="equal",
                    **kwargs):
    """
    Plot a 3D view of the instrument, using the `position` coordinate as the
    detector vector positions.
    Use the `positions` argument to change the vectors used as pixel positions.
    Sliders are added to navigate extra dimensions.
    In the case of event data, on-the-fly histogramming can be performed using
    the `bins` keyword argument, is expected to be a dict containing
    dimensions to be histogrammed as keys, and the number of bins (or an array
    of bin edges) as values.
    If no bins are specified, the data is histogrammed according to the bins
    that wrap the event data when it is loaded
    (see https://scipp.github.io/user-guide/binned-data.html).

    Example:

    import scipp.neutron as sn
    sample = sn.load(filename="PG3_4844_event.nxs")
    sn.instrument_view(sample, bins={'tof': 256})
    """

    from ..plot import plot

    return plot(scipp_obj,
                projection="3d",
                positions=positions,
                aspect=aspect,
                **kwargs)
