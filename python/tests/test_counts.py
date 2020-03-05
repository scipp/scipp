# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
import scipp as sc


def test_round_trip_dataset():
    data = sc.Variable(['tof'], values=[10., 10.], unit=sc.units.counts)
    tof = sc.Variable(['tof'], values=[1., 2., 3.], unit=sc.units.us)

    d = sc.Dataset({'data': data}, coords={'tof': tof})

    dd = sc.density_to_counts(sc.counts_to_density(d, 'tof'), 'tof')
    assert dd == d


def test_round_trip_dataarray():
    data = sc.Variable(['tof'], values=[10., 10.], unit=sc.units.counts)
    tof = sc.Variable(['tof'], values=[1., 2., 3.], unit=sc.units.us)

    d = sc.DataArray(data, coords={'tof': tof})

    dd = sc.density_to_counts(sc.counts_to_density(d, 'tof'), 'tof')
    assert dd == d
