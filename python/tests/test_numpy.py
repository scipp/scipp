# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import scipp as sc


def test_numpy_self_assign():
    expected = sc.Variable(dims=['x'], values=np.arange(99, -1, -1))
    var = sc.Variable(dims=['x'], values=np.arange(100))
    a = np.flip(var.values)
    var.values = a
    assert sc.is_equal(var, expected)
