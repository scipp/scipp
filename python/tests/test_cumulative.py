# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
import scipp as sc

from .common import assert_export


def test_cumsum():
    var = 1 * sc.units.m
    assert_export(sc.cumsum, a=var)
    assert_export(sc.cumsum, a=var, dim='x')
    assert_export(sc.cumsum, a=var, inclusive=False)
    assert_export(sc.cumsum, a=var, dim='x', inclusive=False)
