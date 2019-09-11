# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
from scipp import Dim
import scipp.neutron as sn
import numpy as np


def make_dataset_with_beamline():
    d = sc.Dataset(
        {
            'a': sc.Variable([Dim.Position, Dim.Tof],
                             values=np.random.rand(4, 9))
        },
        coords={
            Dim.Tof:
            sc.Variable([Dim.Tof],
                        values=np.arange(1000.0, 1010.0),
                        unit=sc.units.us),
            Dim.Position:
            sc.Variable(dims=[Dim.Position],
                        shape=(4, ),
                        dtype=sc.dtype.vector_3_double,
                        unit=sc.units.m)
        },
        labels={'component_info': sc.Variable(dtype=sc.dtype.Dataset)})
    d.coords[Dim.Position].values[0] = [1, 0, 0]
    d.coords[Dim.Position].values[1] = [0, 1, 0]
    d.coords[Dim.Position].values[2] = [0, 0, 1]
    d.coords[Dim.Position].values[3] = [-1, 0, 0]
    component_info = sc.Dataset({
        'position':
        sc.Variable(dims=[Dim.Row],
                    shape=(2, ),
                    dtype=sc.dtype.vector_3_double,
                    unit=sc.units.m)
    })
    component_info['position'].values[0] = [0, 0, -10]
    component_info['position'].values[1] = [0, 0, 0]

    d.labels['component_info'].value = component_info
    return d


def test_neutron_convert():
    d = make_dataset_with_beamline()

    dspacing = sn.convert(d, Dim.Tof, Dim.DSpacing)
    # Detailed testing done on the C++ side
    assert dspacing.coords[Dim.DSpacing].unit == sc.units.angstrom


def test_neutron_beamline():
    d = make_dataset_with_beamline()

    assert sn.source_position(d) == sc.Variable(value=[0, 0, -10],
                                                unit=sc.units.m)
    assert sn.sample_position(d) == sc.Variable(value=[0, 0, 0],
                                                unit=sc.units.m)
    assert sn.l1(d) == 10.0 * sc.units.m
    assert sn.l2(d) == sc.Variable(dims=[Dim.Position],
                                   values=np.ones(4),
                                   unit=sc.units.m)
    two_theta = sn.two_theta(d)
    assert two_theta.unit == sc.units.rad
    assert two_theta.dims == [Dim.Position]
    assert sn.scattering_angle(d) == 0.5 * two_theta
