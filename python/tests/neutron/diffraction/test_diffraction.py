# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
import scipp as sc
import numpy as np


def make_simple_tof_dataset():
    return sc.Dataset(
        {
            "counts":
            sc.Variable(dims=['spectrum', 'tof'],
                        unit=sc.units.counts,
                        values=np.array([[1, 2, 3], [4, 5, 6]]))
        },
        coords={
            'tof':
            sc.Variable(dims=['tof'],
                        unit=sc.units.us,
                        values=np.array([4000, 5000, 6100, 7300]))
        })


def make_calibration_dataset():
    return sc.Dataset({
        "tzero":
        sc.Variable(dims=['spectrum'],
                    unit=sc.units.us,
                    values=np.array([1.1, 2.2])),
        "difc":
        sc.Variable(dims=['spectrum'],
                    unit=sc.units.us / sc.units.angstrom,
                    values=np.array([3.3, 4.4]))
    })


def test_neutron_convert_with_calibration():
    tof = make_simple_tof_dataset()
    cal = make_calibration_dataset()

    out = sc.neutron.diffraction.convert_with_calibration(tof, cal)

    assert out["counts"].dims == ['spectrum', 'd-spacing']
    assert out.coords['d-spacing'].unit == sc.units.angstrom


def test_neutron_convert_with_calibration_in_place():
    tof = make_simple_tof_dataset()
    cal = make_calibration_dataset()

    out = sc.neutron.diffraction.convert_with_calibration(tof, cal, tof)

    assert tof["counts"].dims == ['spectrum', 'd-spacing']
    assert tof.coords['d-spacing'].unit == sc.units.angstrom
    assert out == tof
