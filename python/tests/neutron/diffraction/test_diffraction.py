# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
import scipp as sc
from scipp import Dim
import numpy as np


def test_neutron_convert_with_calibration():
    tof = sc.Dataset(
        {
            "counts":
            sc.Variable(dims=[Dim.Spectrum, Dim.Tof],
                        unit=sc.units.counts,
                        values=np.array([[1, 2, 3], [4, 5, 6]]))
        },
        coords={
            Dim.Tof:
            sc.Variable(dims=[Dim.Tof],
                        unit=sc.units.us,
                        values=np.array([4000, 5000, 6100, 7300]))
        })

    cal = sc.Dataset({
        "tzero":
        sc.Variable(dims=[Dim.Spectrum],
                    unit=sc.units.us,
                    values=np.array([1.1, 2.2])),
        "difc":
        sc.Variable(dims=[Dim.Spectrum],
                    unit=sc.units.us / sc.units.angstrom,
                    values=np.array([3.3, 4.4]))
    })

    out = sc.neutron.diffraction.convert_with_calibration(tof, cal)

    assert out["counts"].dims == [Dim.Spectrum, Dim.DSpacing]
    assert out.coords[Dim.DSpacing].unit == sc.units.angstrom
