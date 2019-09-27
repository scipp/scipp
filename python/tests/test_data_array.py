# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import scipp as sc
from scipp import Dim


def test_in_place_binary_with_variable():
    a = sc.DataArray(
        data=sc.Variable([Dim.X], values=np.arange(10.0)),
        coords={Dim.X: sc.Variable([Dim.X], values=np.arange(10.0))})
    copy = a.copy()

    a += 2.0 * sc.units.dimensionless
    a *= 2.0 * sc.units.m
    a -= 4.0 * sc.units.m
    a /= 2.0 * sc.units.m
    assert a == copy


def test_in_place_binary_with_scalar():
    a = sc.DataArray(
        data=sc.Variable([Dim.X], values=[10]),
        coords={Dim.X: sc.Variable([Dim.X], values=[10])})
    copy = a.copy()

    a += 2
    a *= 2
    a -= 4
    a /= 2
    assert a == copy
