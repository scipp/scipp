# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import scipp as sc
import numpy as np


def test_moving_variable_into_dataset():

    d = sc.Dataset()
    var = sc.Variable(['x'], values=np.arange(10000.0))
    d["a"] = sc.detail.move(var)

    assert d.dims == ['x']
    assert d.shape == [10000]
    assert "a" in d


def test_moving_variable_into_dataset_proxies():

    d = sc.Dataset()
    d.coords['x'] = sc.detail.move(sc.Variable(['x'],
                                               values=np.arange(1000.0)))
    d['data'] = 1.0 * sc.units.m
    d['data'].masks["c"] = sc.detail.move(
        sc.Variable(['z'], values=np.random.random(50)))

    assert len(d.coords) == 1
    assert len(d['data'].masks) == 1
    assert 'x' in d.coords
    assert "c" in d['data'].masks
    assert d.coords['x'].shape == [1000]
    assert d['data'].masks["c"].shape == [50]


def test_moving_variable_into_data_array_proxies():

    a = sc.DataArray(data=sc.Variable(['x'], values=np.random.random(1000)))
    a.coords['x'] = sc.detail.move(sc.Variable(['x'],
                                               values=np.arange(1000.0)))
    a.attrs["b"] = sc.detail.move(
        sc.Variable(['x'], values=np.random.random(1000)))
    a.masks["c"] = sc.detail.move(
        sc.Variable(['x'], values=np.random.random(1000)))

    assert a.dims == ['x']
    assert a.shape == [1000]
    assert len(a.meta) == 2
    assert len(a.coords) == 1
    assert len(a.attrs) == 1
    assert len(a.masks) == 1
    assert 'x' in a.coords
    assert "b" in a.attrs
    assert "c" in a.masks


def test_moving_variables_to_data_array():

    var = sc.Variable(['x'], values=np.random.random(1000))
    c = sc.Variable(['x'], values=np.arange(1000.0))
    coords = {'x': c}
    a = sc.detail.move_to_data_array(data=var, coords=coords)

    assert a.dims == ['x']
    assert a.shape == [1000]
    assert 'x' in a.coords


def test_moving_data_array_into_dataset():

    var = sc.Variable(['x'], values=np.random.random(10000))
    c = sc.Variable(['x'], values=np.arange(10000.0))
    coords = {'x': c}
    a = sc.detail.move_to_data_array(data=var, coords=coords)
    d = sc.Dataset()
    d["a"] = sc.detail.move(a)

    assert d.dims == ['x']
    assert d.shape == [10000]
    assert "a" in d
    assert 'x' in d.coords
