# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
from scipp import Dim
import numpy as np


def test_neutron_convert():
    d = sc.Dataset(
            {'a': sc.Variable(
                [Dim.Position, Dim.Tof],
                values=np.random.rand(4, 9))},
            coords={
                Dim.Tof: sc.Variable(
                    [Dim.Tof],
                    values=np.arange(1000.0, 1010.0), unit=sc.units.us),
                Dim.Position: sc.Variable(
                    dims=[Dim.Position],
                    shape=(4,),
                    dtype=sc.dtype.vector_3_double,
                    unit=sc.units.m)},
            labels={'component_info': sc.Variable(dtype=sc.dtype.Dataset)})
    d.labels['component_info'].value = sc.Dataset({'position': sc.Variable(
        dims=[Dim.Row], shape=(2,),
        dtype=sc.dtype.vector_3_double,
        unit=sc.units.m)})

    dspacing = sc.neutron.convert(d, Dim.Tof, Dim.DSpacing)
    # Detailed testing done on the C++ side
    assert dspacing.coords[Dim.DSpacing].unit == sc.units.angstrom
