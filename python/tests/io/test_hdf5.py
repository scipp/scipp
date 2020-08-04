# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
import numpy as np

import tempfile


def check_roundtrip(obj):
    with tempfile.TemporaryDirectory() as path:
        name = f'{path}/test.hdf5'
        obj.to_hdf5(filename=name)
        assert sc.io.open_hdf5(filename=name) == obj


x = sc.Variable(dims=['x'], values=np.arange(4.0), unit=sc.units.m)
y = sc.Variable(dims=['y'], values=np.arange(6.0), unit=sc.units.angstrom)
xy = sc.Variable(dims=['y', 'x'],
                 values=np.random.rand(6, 4),
                 unit=sc.units.kg)


def test_data_array_1d_no_coords():
    a = sc.DataArray(data=x)
    check_roundtrip(a)


def test_data_array_all_units_supported():
    for unit in sc.units.supported_units():
        a = sc.DataArray(data=1.0 * unit)
        check_roundtrip(a)


def test_data_array_1d():
    a = sc.DataArray(data=x,
                     coords={
                         'x': x,
                         'x2': 2.0 * x
                     },
                     masks={
                         'mask1': sc.less(x, 1.5 * sc.units.m),
                         'mask2': sc.less(x, 2.5 * sc.units.m)
                     })
    check_roundtrip(a)


def test_data_array_2d():
    a = sc.DataArray(data=xy,
                     coords={
                         'x': x,
                         'y': y,
                         'x2': 2.0 * x
                     },
                     masks={
                         'mask1': sc.less(x, 1.5 * sc.units.m),
                         'mask2': sc.less(xy, 0.5 * sc.units.kg)
                     })
    check_roundtrip(a)
