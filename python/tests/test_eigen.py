# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import pytest

import scipp as sc


def test_create_variable_0D_vector_3_float64():
    var = sc.Variable(value=[[1, 2, 3]],
                      unit=sc.units.m,
                      dtype=sc.dtype.vector_3_float64)
    np.testing.assert_array_equal(var.value, [1, 2, 3])
    assert var.dtype == sc.dtype.vector_3_float64
    assert var.unit == sc.units.m


def test_create_variable_1D_vector_3_float64():
    var = sc.Variable(dims=['x'],
                      values=[[1, 2, 3], [4, 5, 6]],
                      unit=sc.units.m,
                      dtype=sc.dtype.vector_3_float64)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == ['x']
    assert var.dtype == sc.dtype.vector_3_float64
    assert var.unit == sc.units.m


def test_create_quaternion_float64():
    quat = sc.Quat(np.arange(4.0))
    assert mat.x() == 0.0
    assert mat.y() == 1.0
    assert mat.z() == 2.0
    assert mat.w() == 3.0
    np.testing.assert_array_equal(quat.coeffs(), [0, 1, 2, 3])


def test_create_variable_0D_quaternion_float64():
    quat = sc.Quat(np.arange(4.0))
    var = sc.Variable(value=quat,
                  unit=sc.units.m, dtype=sc.dtype.quaternion_float64)
    np.testing.assert_array_equal(var.value.coeffs(), [0, 1, 2, 3])
    assert var.dtype == sc.dtype.quaternion_float64
    assert var.unit == sc.units.m


def test_create_variable_1D_quaternion_float64():
    var = sc.Variable(['tof'], values=np.random.random([10, 4]),
                  unit=sc.units.us, dtype=sc.dtype.quaternion_float64)
    assert len(var.values) == 10
    assert var.dims == ['tof']
    assert var.dtype == sc.dtype.quaternion_float64
    assert var.unit == sc.units.us
