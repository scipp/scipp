# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import scipp as sc


def test_round_trip_dataset():
    data = sc.Variable(dims=['tof'], values=[10., 10.], unit=sc.units.counts)
    tof = sc.Variable(dims=['tof'], values=[1., 2., 3.], unit=sc.units.us)

    d = sc.Dataset(data={'data': data}, coords={'tof': tof})

    dd = sc.density_to_counts(sc.counts_to_density(d, 'tof'), 'tof')
    assert sc.identical(dd, d)


def test_round_trip_dataarray():
    data = sc.Variable(dims=['tof'], values=[10., 10.], unit=sc.units.counts)
    tof = sc.Variable(dims=['tof'], values=[1., 2., 3.], unit=sc.units.us)

    d = sc.DataArray(data, coords={'tof': tof})

    dd = sc.density_to_counts(sc.counts_to_density(d, 'tof'), 'tof')
    assert sc.identical(dd, d)
