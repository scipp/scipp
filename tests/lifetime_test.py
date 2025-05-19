# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""This file contains tests specific to pybind11 lifetime issues, in particular
py::return_value_policy and py::keep_alive."""

from collections.abc import Callable

import numpy as np
import pytest

import scipp as sc


def test_lifetime_values_of_py_array_t_item() -> None:
    d = sc.Dataset(data={'a': sc.Variable(dims=['x'], values=np.arange(10))})
    assert d['a'].values[-1] == 9


def test_lifetime_values_of_py_array_t_item_of_temporary() -> None:
    d = sc.Dataset(data={'a': sc.Variable(dims=['x'], values=np.arange(10))})
    vals = (d + d)['a'].values
    d + d  # do something allocating memory to trigger potential segfault
    assert vals[-1] == 2 * 9


def test_lifetime_values_of_item() -> None:
    d = sc.Dataset(data={'a': sc.Variable(dims=['x'], values=["aa", "bb", "cc"])})
    assert d['a'].values[2] == "cc"


def test_lifetime_values_of_item_of_temporary() -> None:
    d = sc.Dataset(
        data={'a': sc.Variable(dims=['x'], values=np.arange(3))},
        coords={'x': sc.Variable(dims=['x'], values=["aa", "bb", "cc"])},
    )
    vals = (d + d).coords['x'].values
    d + d  # do something allocating memory to trigger potential segfault
    assert vals[2] == "cc"


def test_lifetime_coords_of_temporary() -> None:
    var = sc.Variable(dims=['x'], values=np.arange(10))
    d = sc.Dataset(data={'a': var}, coords={'x': var, 'aux': var})
    assert d.coords['x'].values[-1] == 9
    assert d['a'].coords['x'].values[-1] == 9
    assert d['x', 1:]['a'].coords['x'].values[-1] == 9
    assert (d + d).coords['x'].values[-1] == 9
    assert (d + d).coords['aux'].values[-1] == 9


def test_lifetime_items_iter() -> None:
    var = sc.Variable(dims=['x'], values=np.arange(10))
    d = sc.Dataset(data={'a': var}, coords={'x': var, 'aux': var})
    for _, item in (d + d).items():  # noqa: PERF102
        assert sc.identical(item.data, var + var)
    for _, coord in (d + d).coords.items():  # noqa: PERF102
        assert sc.identical(coord, var)
    for _, item in d['x', 1:5].items():  # noqa: PERF102
        assert sc.identical(item.data, var['x', 1:5])
    for _, coord in d['x', 1:5].coords.items():  # noqa: PERF102
        assert sc.identical(coord, var['x', 1:5])
    for _, item in (d + d)['x', 1:5].items():  # noqa: PERF102
        assert sc.identical(item.data, (var + var)['x', 1:5])
    for _, coord in (d + d)['x', 1:5].coords.items():  # noqa: PERF102
        assert sc.identical(coord, var['x', 1:5])


def test_lifetime_single_value() -> None:
    d = sc.Dataset(data={'a': sc.Variable(dims=['x'], values=np.arange(10))})
    var = sc.scalar(d)
    assert var.value['a'].values[-1] == 9
    assert var.copy().values['a'].values[-1] == 9


def test_lifetime_coord_values() -> None:
    var = sc.arange('x', 10)
    d = sc.Dataset({'a': var}, coords={'x': var})
    values = d.coords['x'].values
    d += d
    assert np.array_equal(values, var.values)


def test_lifetime_scalar_py_object() -> None:
    var = sc.scalar([1] * 100000)
    assert var.dtype == sc.DType.PyObject
    val = var.copy().value
    import gc

    gc.collect()
    var.copy()  # do something allocating memory to trigger potential segfault
    assert val[-1] == 1


def test_lifetime_scalar_nested() -> None:
    var = sc.scalar(sc.array(dims=['x'], values=np.array([0, 1])))
    arr = var.value.values
    var.value = sc.array(dims=['x'], values=np.array([2, 3]))
    import gc

    gc.collect()
    np.testing.assert_array_equal(arr, [0, 1])


def test_lifetime_scalar_nested_string() -> None:
    var = sc.scalar(sc.array(dims=['x'], values=np.array(['abc', 'def'])))
    arr = var.value.values
    var.value = sc.array(dims=['x'], values=np.array(['ghi', 'jkl']))
    import gc

    gc.collect()
    np.testing.assert_array_equal(arr, ['abc', 'def'])


def test_lifetime_scalar() -> None:
    elem = sc.Variable(dims=['x'], values=np.arange(100000))
    var = sc.scalar(elem)
    assert sc.identical(var.values, elem)
    vals = var.copy().values
    import gc

    gc.collect()
    var.copy()  # do something allocating memory to trigger potential segfault
    assert sc.identical(vals, elem)


def test_lifetime_array() -> None:
    var = sc.Variable(dims=['x'], values=np.arange(5))
    array = var.values
    del var
    import gc

    gc.collect()
    # do something allocating memory to trigger potential segfault
    sc.Variable(dims=['x'], values=np.arange(5, 10))
    assert np.array_equal(array, np.arange(5))


def test_lifetime_string_array() -> None:
    var = sc.Variable(dims=['x'], values=['ab', 'c'] * 100000)
    assert var.values[100000] == 'ab'
    vals = var.copy().values
    import gc

    gc.collect()
    var.copy()  # do something allocating memory to trigger potential segfault
    assert vals[100000] == 'ab'


@pytest.mark.parametrize("func", [sc.abs, sc.sqrt, sc.reciprocal, sc.nan_to_num])
def test_lifetime_out_arg(func: Callable[..., sc.Variable]) -> None:
    var = 1.0 * sc.units.one
    var = func(var, out=var)
    var *= 1.0  # var would be an invalid view is keep_alive not correct


@pytest.mark.parametrize("func", [sc.sin, sc.cos, sc.tan])
def test_lifetime_trigonometry_out_arg(func: Callable[..., sc.Variable]) -> None:
    var = 1.0 * sc.units.rad
    var = func(var, out=var)
    var *= 1.0  # var would be an invalid view is keep_alive not correct


@pytest.mark.parametrize("func", [sc.asin, sc.acos, sc.atan])
def test_lifetime_inverse_trigonometry_out_arg(
    func: Callable[..., sc.Variable],
) -> None:
    var = 1.0 * sc.units.one
    var = func(var, out=var)
    var *= 1.0  # var would be an invalid view is keep_alive not correct


def test_lifetime_atan2_out_arg() -> None:
    var = 1.0 * sc.units.one
    var = sc.atan2(y=var, x=var, out=var)
    var *= 1.0  # var would be an invalid view is keep_alive not correct
