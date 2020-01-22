# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import scipp as sc
from scipp import Dim
import numpy as np


def test_moving_variable_into_dataset():

    d = sc.Dataset()
    var = sc.Variable([Dim.Y], values=np.arange(10000.0))
    d['a'] = sc.detail.move(var)

    assert d.dims == [Dim.Y]
    assert var.shape == [0]
    assert var.dims == [Dim.X]


def test_moving_variable_into_dataset_proxies():

    d = sc.Dataset()
    d.coords[Dim.X] = sc.detail.move(
        sc.Variable([Dim.X], values=np.arange(1000.0)))
    d.labels['a'] = sc.detail.move(
        sc.Variable([Dim.Y], values=np.random.random(100)))
    d.attrs['b'] = sc.detail.move(
        sc.Variable([Dim.Z], values=np.random.random(50)))
    d.masks['c'] = sc.detail.move(
        sc.Variable([Dim.Z], values=np.random.random(50)))

    assert Dim.X in d.dims
    assert Dim.Y in d.dims
    assert Dim.Z in d.dims


def test_moving_variable_into_data_array_proxies():

    d = sc.DataArray(data=sc.Variable([Dim.X], values=np.random.random(1000)))
    d.coords[Dim.X] = sc.detail.move(
        sc.Variable([Dim.X], values=np.arange(1000.0)))
    d.labels['a'] = sc.detail.move(
        sc.Variable([Dim.X], values=np.random.random(1000)))
    d.attrs['b'] = sc.detail.move(
        sc.Variable([Dim.X], values=np.random.random(1000)))
    d.masks['c'] = sc.detail.move(
        sc.Variable([Dim.X], values=np.random.random(1000)))

    assert Dim.X in d.dims
    assert len(d.coords) == 1
    assert len(d.labels) == 1
    assert len(d.attrs) == 1
    assert len(d.masks) == 1


def test_moving_variables_to_data_array():

    var = sc.Variable([Dim.X], values=np.random.random(1000))
    c = sc.Variable([Dim.X], values=np.arange(1000.0))
    coords = {Dim.X: c}
    array = sc.detail.move_to_data_array(data=var, coords=coords)

    assert array.dims == [Dim.X]
    assert array.shape == [1000]


def test_moving_data_array_into_dataset():

    var = sc.Variable([Dim.X], values=np.random.random(10000))
    c = sc.Variable([Dim.X], values=np.arange(10000.0))
    coords = {Dim.X: c}
    array = sc.detail.move_to_data_array(data=var, coords=coords)
    d = sc.Dataset()
    d['a'] = sc.detail.move(array)
