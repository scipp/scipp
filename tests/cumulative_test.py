# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
import scipp as sc

from .common import assert_export


def test_cumsum():
    var = sc.arange('xx', 10, unit='m')
    assert_export(sc.cumsum, a=var)
    assert_export(sc.cumsum, a=var, dim='xx')
    assert_export(sc.cumsum, a=var, mode='exclusive')
    assert_export(sc.cumsum, a=var, dim='xx', mode='exclusive')
