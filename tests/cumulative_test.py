# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import scipp as sc

from .common import assert_export


def test_cumsum():
    var = 1 * sc.units.m
    assert_export(sc.cumsum, a=var)
    assert_export(sc.cumsum, a=var, dim='x')
    assert_export(sc.cumsum, a=var, mode='inclusive')
    assert_export(sc.cumsum, a=var, dim='x', mode='inclusive')
