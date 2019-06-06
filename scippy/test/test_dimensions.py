# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2019 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import scippy as sp
from scippy import Dim


def test_empty():
    dims = sp.Dimensions()
    assert len(dims.labels) == 0
    assert len(dims.shape) == 0


def test_dense():
    dims = sp.Dimensions([Dim.X, Dim.Y], [2, 3])
    assert len(dims.labels) == 2
    assert len(dims.shape) == 2
    assert dims.labels[0] == Dim.X
    assert dims.labels[1] == Dim.Y
    assert dims.shape[0] == 2
    assert dims.shape[1] == 3


def test_comparison():
    a = sp.Dimensions(labels=[Dim.X, Dim.Y], shape=[2, 3])
    a2 = sp.Dimensions(labels=[Dim.X, Dim.Y], shape=[2, 3])
    b = sp.Dimensions(labels=[Dim.X, Dim.Y], shape=[2, 4])
    assert a == a
    assert a == a2
    assert a != b


def test_lifetime():
    # pybind11 is keeping the parent object (Dimensions) alive
    shape = sp.Dimensions(labels=[Dim.X, Dim.Y], shape=[2, 3]).shape
    assert len(shape) == 2
    assert shape[1] == 3
