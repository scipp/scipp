# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
from scipp import Dim


def test_all():
    var = sc.Variable(['x'], values=[True, False, True])
    assert sc.all(var, 'x') == sc.Variable(value=False)


def test_any():
    var = sc.Variable(['x'], values=[True, False, True])
    assert sc.any(var, 'x') == sc.Variable(value=True)


def test_min():
    var = sc.Variable(['x'], values=[1.0, 2.0, 3.0])
    assert sc.min(var, 'x') == sc.Variable(value=1.0)


def test_max():
    var = sc.Variable(['x'], values=[1.0, 2.0, 3.0])
    assert sc.max(var, 'x') == sc.Variable(value=3.0)
