# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
import scipp as sc
from scipp import Dim
import numpy as np


def test_moving_variable_into_dataset():

    d = sc.Dataset()
    var = sc.Variable([Dim.X], values=np.arange(10000.0))
    d["a"] = sc.detail.move(var)

    assert d.dims == [Dim.X]
    assert d.shape == [10000]
    assert "a" in d


def test_moving_variable_into_dataset_proxies():

    d = sc.Dataset()
    d.coords[Dim.X] = sc.detail.move(
        sc.Variable([Dim.X], values=np.arange(1000.0)))
    d.labels["a"] = sc.detail.move(
        sc.Variable([Dim.Y], values=np.random.random(100)))
    d.attrs["b"] = sc.detail.move(
        sc.Variable([Dim.Z], values=np.random.random(50)))
    d.masks["c"] = sc.detail.move(
        sc.Variable([Dim.Z], values=np.random.random(50)))

    assert Dim.X in d.dims
    assert Dim.Y in d.dims
    assert Dim.Z in d.dims
    assert len(d.coords) == 1
    assert len(d.labels) == 1
    assert len(d.attrs) == 1
    assert len(d.masks) == 1
    assert Dim.X in d.coords
    assert "a" in d.labels
    assert "b" in d.attrs
    assert "c" in d.masks
    assert d.coords[Dim.X].shape == [1000]
    assert d.labels["a"].shape == [100]
    assert d.attrs["b"].shape == [50]
    assert d.masks["c"].shape == [50]


def test_moving_variable_into_data_array_proxies():

    a = sc.DataArray(data=sc.Variable([Dim.X], values=np.random.random(1000)))
    a.coords[Dim.X] = sc.detail.move(
        sc.Variable([Dim.X], values=np.arange(1000.0)))
    a.labels["a"] = sc.detail.move(
        sc.Variable([Dim.X], values=np.random.random(1000)))
    a.attrs["b"] = sc.detail.move(
        sc.Variable([Dim.X], values=np.random.random(1000)))
    a.masks["c"] = sc.detail.move(
        sc.Variable([Dim.X], values=np.random.random(1000)))

    assert a.dims == [Dim.X]
    assert a.shape == [1000]
    assert len(a.coords) == 1
    assert len(a.labels) == 1
    assert len(a.attrs) == 1
    assert len(a.masks) == 1
    assert Dim.X in a.coords
    assert "a" in a.labels
    assert "b" in a.attrs
    assert "c" in a.masks


def test_moving_variables_to_data_array():

    var = sc.Variable([Dim.X], values=np.random.random(1000))
    c = sc.Variable([Dim.X], values=np.arange(1000.0))
    coords = {Dim.X: c}
    a = sc.detail.move_to_data_array(data=var, coords=coords)

    assert a.dims == [Dim.X]
    assert a.shape == [1000]
    assert Dim.X in a.coords


def test_moving_data_array_into_dataset():

    var = sc.Variable([Dim.X], values=np.random.random(10000))
    c = sc.Variable([Dim.X], values=np.arange(10000.0))
    coords = {Dim.X: c}
    a = sc.detail.move_to_data_array(data=var, coords=coords)
    d = sc.Dataset()
    d["a"] = sc.detail.move(a)

    assert d.dims == [Dim.X]
    assert d.shape == [10000]
    assert "a" in d
    assert Dim.X in d.coords
