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
        {'a': sc.Variable([Dim.X], values=np.arange(3))},
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


def test_lifetime_coord_values():
    var = sc.Variable([Dim.X], values=np.arange(10))
    d = sc.Dataset(coords={Dim.X: var})
    values = d.coords[Dim.X].values
    d += d
    assert np.array_equal(values, var.values)


def test_lifetime_scalar_py_object():
    var = sc.Variable(value=[1] * 100000)
    assert var.dtype == sc.dtype.PyObject
    val = var.copy().value
    import gc
    gc.collect()
    var.copy()  # do something allocating memory to trigger potential segfault
    assert val[-1] == 1


def test_lifetime_scalar():
    elem = sc.Variable([Dim.X], values=np.arange(100000))
    var = sc.Variable(value=elem)
    assert var.values == elem
    vals = var.copy().values
    import gc
    gc.collect()
    var.copy()  # do something allocating memory to trigger potential segfault
    assert var.values == elem


def test_lifetime_string_array():
    var = sc.Variable([Dim.X], values=['ab', 'c'] * 100000)
    assert var.values[100000] == 'ab'
    vals = var.copy().values
    import gc
    gc.collect()
    var.copy()  # do something allocating memory to trigger potential segfault
    assert vals[100000] == 'ab'
