# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scipp as sc
from scipp import Dim
import pytest


def test_large_variable():
    for n in [10, 100, 1000, 10000]:
        var = sc.Variable(['x', 'y'], shape=(n, n))
    assert len(sc.make_svg(var)) < 100000


def test_too_many_variable_dimensions():
    var = sc.Variable(['x', 'y', 'z', Dim.Time], shape=(1, 1, 1, 1))
    with pytest.raises(RuntimeError):
        sc.make_svg(var)


def test_too_many_dataset_dimensions():
    d = sc.Dataset({
        'xy': sc.Variable(['x', 'y'], shape=(1, 1)),
        'zt': sc.Variable(['z', Dim.Time], shape=(1, 1))
    })
    with pytest.raises(RuntimeError):
        sc.make_svg(d)
