# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc


def test_variable_0D_vector_3_float64_from_list():
    var = sc.Variable(value=[1, 2, 3],
                      unit=sc.units.m,
                      dtype=sc.dtype.vector_3_float64)
    np.testing.assert_array_equal(var.value, [1, 2, 3])
    assert var.dtype == sc.dtype.vector_3_float64
    assert var.unit == sc.units.m


def test_variable_0D_vector_3_float64_from_numpy():
    var = sc.Variable(value=np.array([1, 2, 3]),
                      unit=sc.units.m,
                      dtype=sc.dtype.vector_3_float64)
    np.testing.assert_array_equal(var.value, [1, 2, 3])
    assert var.dtype == sc.dtype.vector_3_float64
    assert var.unit == sc.units.m


def test_variable_1D_vector_3_float64_from_list():
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


def test_variable_1D_vector_3_float64_from_numpy():
    var = sc.Variable(dims=['x'],
                      values=np.array([[1, 2, 3], [4, 5, 6]]),
                      unit=sc.units.m,
                      dtype=sc.dtype.vector_3_float64)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == ['x']
    assert var.dtype == sc.dtype.vector_3_float64
    assert var.unit == sc.units.m


def test_quaternion_float64_from_list():
    quat = sc.Quat([1, 2, 3, 4])
    assert quat.x() == 1.0
    assert quat.y() == 2.0
    assert quat.z() == 3.0
    assert quat.w() == 4.0
    np.testing.assert_array_equal(quat.coeffs(), [1, 2, 3, 4])


def test_quaternion_float64_from_numpy():
    quat = sc.Quat(np.arange(4.0))
    assert quat.x() == 0.0
    assert quat.y() == 1.0
    assert quat.z() == 2.0
    assert quat.w() == 3.0
    np.testing.assert_array_equal(quat.coeffs(), [0, 1, 2, 3])


def test_variable_0D_quaternion_float64_from_quat():
    quat = sc.Quat(np.arange(4.0))
    var = sc.Variable(value=quat, unit=sc.units.m)
    np.testing.assert_array_equal(var.value.coeffs(), [0, 1, 2, 3])
    assert var.dtype == sc.dtype.quaternion_float64
    assert var.unit == sc.units.m


def test_variable_0D_quaternion_float64_from_list():
    var = sc.Variable(value=[1, 2, 3, 4],
                      unit=sc.units.m,
                      dtype=sc.dtype.quaternion_float64)
    np.testing.assert_array_equal(var.value.coeffs(), [1, 2, 3, 4])
    assert var.dtype == sc.dtype.quaternion_float64
    assert var.unit == sc.units.m


def test_variable_0D_quaternion_float64_from_numpy():
    var = sc.Variable(value=np.arange(4.0),
                      unit=sc.units.m,
                      dtype=sc.dtype.quaternion_float64)
    np.testing.assert_array_equal(var.value.coeffs(), [0, 1, 2, 3])
    assert var.dtype == sc.dtype.quaternion_float64
    assert var.unit == sc.units.m


def test_variable_1D_quaternion_float64_from_list():
    var = sc.Variable(['tof'],
                      values=[[1, 2, 3, 4], [5, 6, 7, 8], [9, 10, 11, 12]],
                      unit=sc.units.us,
                      dtype=sc.dtype.quaternion_float64)
    assert len(var.values) == 3
    np.testing.assert_array_equal(var.values[0], [1, 2, 3, 4])
    np.testing.assert_array_equal(var.values[1], [5, 6, 7, 8])
    np.testing.assert_array_equal(var.values[2], [9, 10, 11, 12])
    assert var.dims == ['tof']
    assert var.dtype == sc.dtype.quaternion_float64
    assert var.unit == sc.units.us


def test_variable_1D_quaternion_float64_from_numpy():
    data = np.array([np.arange(4.0), np.arange(5.0, 9.0), np.arange(1.0, 5.0)])
    var = sc.Variable(['tof'],
                      values=data,
                      unit=sc.units.us,
                      dtype=sc.dtype.quaternion_float64)
    assert len(var.values) == 3
    np.testing.assert_array_equal(var.values[0], [0, 1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [5, 6, 7, 8])
    np.testing.assert_array_equal(var.values[2], [1, 2, 3, 4])
    assert var.dims == ['tof']
    assert var.dtype == sc.dtype.quaternion_float64
    assert var.unit == sc.units.us
