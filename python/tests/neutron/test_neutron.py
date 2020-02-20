# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
from scipp import Dim
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
                        dtype=sc.dtype.vector_3_float64,
                        unit=sc.units.m)
        })
    d.coords[Dim.Position].values[0] = [1, 0, 0]
    d.coords[Dim.Position].values[1] = [0, 1, 0]
    d.coords[Dim.Position].values[2] = [0, 0, 1]
    d.coords[Dim.Position].values[3] = [-1, 0, 0]

    d.labels['source_position'] = sc.Variable(value=np.array([0, 0, -10]),
                                              dtype=sc.dtype.vector_3_float64,
                                              unit=sc.units.m)
    d.labels['sample_position'] = sc.Variable(value=np.array([0, 0, 0]),
                                              dtype=sc.dtype.vector_3_float64,
                                              unit=sc.units.m)
    return d


def test_neutron_convert():
    d = make_dataset_with_beamline()

    dspacing = sc.neutron.convert(d, Dim.Tof, Dim.DSpacing)
    # Detailed testing done on the C++ side
    assert dspacing.coords[Dim.DSpacing].unit == sc.units.angstrom


def test_neutron_convert_out_arg():
    d = make_dataset_with_beamline()

    dspacing = sc.neutron.convert(d, Dim.Tof, Dim.DSpacing, out=d)
    assert dspacing.coords[Dim.DSpacing].unit == sc.units.angstrom
    assert dspacing is d


def test_neutron_beamline():
    d = make_dataset_with_beamline()

    assert sc.neutron.source_position(d) == sc.Variable(
        value=np.array([0, 0, -10]),
        dtype=sc.dtype.vector_3_float64,
        unit=sc.units.m)
    assert sc.neutron.sample_position(d) == sc.Variable(
        value=np.array([0, 0, 0]),
        dtype=sc.dtype.vector_3_float64,
        unit=sc.units.m)
    assert sc.neutron.l1(d) == 10.0 * sc.units.m
    assert sc.neutron.l2(d) == sc.Variable(dims=[Dim.Position],
                                           values=np.ones(4),
                                           unit=sc.units.m)
    two_theta = sc.neutron.two_theta(d)
    assert two_theta.unit == sc.units.rad
    assert two_theta.dims == [Dim.Position]
    assert sc.neutron.scattering_angle(d) == 0.5 * two_theta


def test_neutron_instrument_view_3d():
    d = make_dataset_with_beamline()
    sc.neutron.instrument_view(d["a"])


def test_neutron_instrument_view_cylindrical():
    d = make_dataset_with_beamline()
    sc.neutron.instrument_view(d["a"], projection="Cylindrical X")


def test_neutron_instrument_view_spherical():
    d = make_dataset_with_beamline()
    sc.neutron.instrument_view(d["a"], projection="Spherical Y")


def test_neutron_instrument_view_with_dataset():
    d = make_dataset_with_beamline()
    d['b'] = sc.Variable([Dim.Position, Dim.Tof],
                         values=np.arange(36.).reshape(4, 9))
    sc.neutron.instrument_view(d)
