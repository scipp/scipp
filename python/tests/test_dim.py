# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from scipp import Dim


def test_dim():
    assert Dim.X == 'x'
    assert Dim.X != 'y'


def test_dim_from_string():
    a = Dim('a')
    b = Dim('b')
    a2 = Dim('a')
    assert str(a) == 'a'
    assert a == a
    assert a != b
    assert a == a2


def test_dim_builtin_from_string():
    x = Dim('x')
    assert str(x) == 'x'
    assert x == 'x'
    assert x != 'y'
