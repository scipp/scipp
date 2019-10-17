# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
import scipp as sc
from scipp import Dim


def test_round_trip_dataset():
    data = sc.Variable([Dim.Tof], values=[10., 10.], unit=sc.units.counts)
    tof = sc.Variable([Dim.Tof], values=[1., 2., 3.], unit=sc.units.us)

    d = sc.Dataset({'data': data}, coords={Dim.Tof: tof})

    dd = sc.density_to_counts(sc.counts_to_density(d, Dim.Tof), Dim.Tof)
    assert dd == d


def test_round_trip_dataarray():
    data = sc.Variable([Dim.Tof], values=[10., 10.], unit=sc.units.counts)
    tof = sc.Variable([Dim.Tof], values=[1., 2., 3.], unit=sc.units.us)

    d = sc.DataArray(data, coords={Dim.Tof: tof})

    dd = sc.density_to_counts(sc.counts_to_density(d, Dim.Tof), Dim.Tof)
    assert dd == d
