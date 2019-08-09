# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import scipp as sc
from scipp import Dim
"""This file contains tests specific to pybind11 lifetime issues, in particular
py::return_value_policy and py::keep_alive."""


def test_lifetime_values_of_py_array_t_item():
    d = sc.Dataset({'a': sc.Variable([Dim.X], values=np.arange(10))})
    assert d['a'].values[-1] == 9


def test_lifetime_values_of_py_array_t_item_of_temporary():
    d = sc.Dataset({'a': sc.Variable([Dim.X], values=np.arange(10))})
    vals = (d + d)['a'].values
    d + d  # do something allocating memory to trigger potential segfault
    assert vals[-1] == 2 * 9


def test_lifetime_values_of_item():
    d = sc.Dataset({'a': sc.Variable([Dim.X], values=["aa", "bb", "cc"])})
    assert d['a'].values[2] == "cc"


def test_lifetime_values_of_item_of_temporary():
    d = sc.Dataset(
        coords={Dim.X: sc.Variable([Dim.X], values=["aa", "bb", "cc"])})
    vals = (d + d).coords[Dim.X].values
    d + d  # do something allocating memory to trigger potential segfault
    assert vals[2] == "cc"


def test_lifetime_coords_of_temporary():
    var = sc.Variable(dims=[Dim.X], values=np.arange(10))
    d = sc.Dataset({'a': var}, coords={Dim.X: var}, labels={'aux': var})
    assert d.coords[Dim.X].values[-1] == 9
    assert d['a'].coords[Dim.X].values[-1] == 9
    assert d[Dim.X, 1:]['a'].coords[Dim.X].values[-1] == 9
    assert (d + d).coords[Dim.X].values[-1] == 9
    assert (d + d).labels['aux'].values[-1] == 9


def test_lifetime_iter():
    var = sc.Variable(dims=[Dim.X], values=np.arange(10))
    d = sc.Dataset({'a': var}, coords={Dim.X: var}, labels={'aux': var})
    for name, item in d + d:
        assert item.data == var + var
    for dim, coord in (d + d).coords:
        assert coord == var
    for name, item in d[Dim.X, 1:5]:
        assert item.data == var[Dim.X, 1:5]
    for dim, coord in d[Dim.X, 1:5].coords:
        assert coord == var[Dim.X, 1:5]
    for name, item in (d + d)[Dim.X, 1:5]:
        assert item.data == (var + var)[Dim.X, 1:5]
    for dim, coord in (d + d)[Dim.X, 1:5].coords:
        assert coord == var[Dim.X, 1:5]


def test_lifetime_single_value():
    d = sc.Dataset({'a': sc.Variable([Dim.X], values=np.arange(10))})
    var = sc.Variable(d)
    assert var.value['a'].values[-1] == 9
    assert var.copy().values['a'].values[-1] == 9
