# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc

from .common import assert_export


def test_filter():
    assert_export(sc.filter,
                  data=sc.DataArray(1.0 * sc.units.m),
                  filter="x",
                  interval=sc.Variable())
    assert_export(sc.filter,
                  data=sc.DataArray(1.0 * sc.units.m),
                  filter="x",
                  interval=sc.Variable(),
                  keep_attrs=False)
