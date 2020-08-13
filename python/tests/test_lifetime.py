# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import numpy as np
import scipp as sc
import pytest
"""This file contains tests specific to pybind11 lifetime issues, in particular
py::return_value_policy and py::keep_alive."""


def test_lifetime_values_of_py_array_t_item():
    d = sc.Dataset({'a': sc.Variable(['x'], values=np.arange(10))})
    assert d['a'].values[-1] == 9


def test_lifetime_values_of_py_array_t_item_of_temporary():
    d = sc.Dataset({'a': sc.Variable(['x'], values=np.arange(10))})
    vals = (d + d)['a'].values
    d + d  # do something allocating memory to trigger potential segfault
    assert vals[-1] == 2 * 9


def test_lifetime_values_of_item():
    d = sc.Dataset({'a': sc.Variable(['x'], values=["aa", "bb", "cc"])})
    assert d['a'].values[2] == "cc"


def test_lifetime_values_of_item_of_temporary():
    d = sc.Dataset({'a': sc.Variable(['x'], values=np.arange(3))},
                   coords={'x': sc.Variable(['x'], values=["aa", "bb", "cc"])})
    vals = (d + d).coords['x'].values
    d + d  # do something allocating memory to trigger potential segfault
    assert vals[2] == "cc"


def test_lifetime_coords_of_temporary():
    var = sc.Variable(dims=['x'], values=np.arange(10))
    d = sc.Dataset({'a': var}, coords={'x': var, 'aux': var})
    assert d.coords['x'].values[-1] == 9
    assert d['a'].coords['x'].values[-1] == 9
    assert d['x', 1:]['a'].coords['x'].values[-1] == 9
    assert (d + d).coords['x'].values[-1] == 9
    assert (d + d).coords['aux'].values[-1] == 9


def test_lifetime_items_iter():
    var = sc.Variable(dims=['x'], values=np.arange(10))
    d = sc.Dataset({'a': var}, coords={'x': var, 'aux': var})
    for key, item in (d + d).items():
        assert item.data == var + var
    for dim, coord in (d + d).coords.items():
        assert coord == var
    for key, item in d['x', 1:5].items():
        assert item.data == var['x', 1:5]
    for dim, coord in d['x', 1:5].coords.items():
        assert coord == var['x', 1:5]
    for key, item in (d + d)['x', 1:5].items():
        assert item.data == (var + var)['x', 1:5]
    for dim, coord in (d + d)['x', 1:5].coords.items():
        assert coord == var['x', 1:5]


def test_lifetime_single_value():
    d = sc.Dataset({'a': sc.Variable(['x'], values=np.arange(10))})
    var = sc.Variable(d)
    assert var.value['a'].values[-1] == 9
    assert var.copy().values['a'].values[-1] == 9


def test_lifetime_coord_values():
    var = sc.Variable(['x'], values=np.arange(10))
    d = sc.Dataset(coords={'x': var})
    values = d.coords['x'].values
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
    elem = sc.Variable(['x'], values=np.arange(100000))
    var = sc.Variable(value=elem)
    assert var.values == elem
    vals = var.copy().values
    import gc
    gc.collect()
    var.copy()  # do something allocating memory to trigger potential segfault
    assert vals == elem


def test_lifetime_string_array():
    var = sc.Variable(['x'], values=['ab', 'c'] * 100000)
    assert var.values[100000] == 'ab'
    vals = var.copy().values
    import gc
    gc.collect()
    var.copy()  # do something allocating memory to trigger potential segfault
    assert vals[100000] == 'ab'


@pytest.mark.parametrize("func",
                         [sc.abs, sc.sqrt, sc.reciprocal, sc.nan_to_num])
def test_lifetime_out_arg(func):
    var = 1.0 * sc.units.one
    var = func(var, out=var)
    var *= 1.0  # var would be an invalid view is keep_alive not correct


@pytest.mark.parametrize("func", [sc.sin, sc.cos, sc.tan])
def test_lifetime_trigonometry_out_arg(func):
    var = 1.0 * sc.units.rad
    var = func(var, out=var)
    var *= 1.0  # var would be an invalid view is keep_alive not correct


@pytest.mark.parametrize("func", [sc.asin, sc.acos, sc.atan])
def test_lifetime_inverse_trigonometry_out_arg(func):
    var = 1.0 * sc.units.one
    var = func(var, out=var)
    var *= 1.0  # var would be an invalid view is keep_alive not correct


def test_lifetime_atan2_out_arg():
    var = 1.0 * sc.units.one
    var = sc.atan2(var, var, out=var)
    var *= 1.0  # var would be an invalid view is keep_alive not correct


@pytest.mark.parametrize("func", [sc.mean, sc.sum])
def test_lifetime_reduction_out_arg(func):
    var = sc.Variable(dims=['x'], values=[1, 2, 3])
    out = 0.0 * sc.units.one
    out = func(var, 'x', out=out)
    out *= 1.0  # out would be an invalid view is keep_alive not correct
