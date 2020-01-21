# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
from scipp import Dim


def test_all():
    var = sc.Variable([Dim.X], values=[True, False, True])
    assert sc.all(var, Dim.X) == sc.Variable(value=False)


def test_any():
    var = sc.Variable([Dim.X], values=[True, False, True])
    assert sc.any(var, Dim.X) == sc.Variable(value=True)


def test_min():
    var = sc.Variable([Dim.X], values=[1, 2, 3])
    assert sc.min(var, Dim.X) == sc.Variable(value=1)


def test_max():
    var = sc.Variable([Dim.X], values=[1, 2, 3])
    assert sc.max(var, Dim.X) == sc.Variable(value=3)
