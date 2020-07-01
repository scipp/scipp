# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
import numpy as np


def test_all():
    var = sc.Variable(['x', 'y'],
                      values=np.array([True, True, True, False]).reshape(2, 2))
    assert sc.all(var) == sc.Variable(value=False)


def test_all_with_dim():
    var = sc.Variable(['x', 'y'],
                      values=np.array([True, True, True, False]).reshape(2, 2))
    assert sc.all(var, 'x') == sc.Variable(dims=['y'], values=[True, False])
    assert sc.all(var, 'y') == sc.Variable(dims=['x'], values=[True, False])


def test_any():
    var = sc.Variable(['x', 'y'],
                      values=np.array([True, True, True, False]).reshape(2, 2))
    assert sc.any(var) == sc.Variable(value=True)


def test_any_with_dim():
    var = sc.Variable(['x', 'y'],
                      values=np.array([True, True, False,
                                       False]).reshape(2, 2))
    assert sc.any(var, 'x') == sc.Variable(dims=['y'], values=[True, True])
    assert sc.any(var, 'y') == sc.Variable(dims=['x'], values=[True, False])


def test_min():
    var = sc.Variable(['x'], values=[1.0, 2.0, 3.0])
    assert sc.min(var, 'x') == sc.Variable(value=1.0)


def test_max():
    var = sc.Variable(['x'], values=[1.0, 2.0, 3.0])
    assert sc.max(var, 'x') == sc.Variable(value=3.0)
