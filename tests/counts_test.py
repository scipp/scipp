# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import scipp as sc


def test_round_trip_dataset() -> None:
    data = sc.Variable(dims=['tof'], values=[10.0, 10.0], unit=sc.units.counts)
    tof = sc.Variable(dims=['tof'], values=[1.0, 2.0, 3.0], unit=sc.units.us)

    d = sc.Dataset(data={'data': data}, coords={'tof': tof})

    dd = sc.density_to_counts(sc.counts_to_density(d, 'tof'), 'tof')
    assert sc.identical(dd, d)


def test_round_trip_dataarray() -> None:
    data = sc.Variable(dims=['tof'], values=[10.0, 10.0], unit=sc.units.counts)
    tof = sc.Variable(dims=['tof'], values=[1.0, 2.0, 3.0], unit=sc.units.us)

    d = sc.DataArray(data, coords={'tof': tof})

    dd = sc.density_to_counts(sc.counts_to_density(d, 'tof'), 'tof')
    assert sc.identical(dd, d)
