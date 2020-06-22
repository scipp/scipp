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


def test_matrix_from_quat_coeffs_list():
    sc.rotation_matrix_from_quaternion_coeffs([1, 2, 3, 4])


def test_matrix_from_quat_coeffs_numpy():
    sc.rotation_matrix_from_quaternion_coeffs(np.arange(4))


def test_variable_0D_matrix():
    # Use known rotation (180 deg around z) to check correct construction
    rot = sc.Variable(value=sc.rotation_matrix_from_quaternion_coeffs(
        [0, 0, 1, 0]),
                      unit=sc.units.one,
                      dtype=sc.dtype.matrix_3_float64)
    vec = sc.Variable(value=[1, 2, 3],
                      unit=sc.units.m,
                      dtype=sc.dtype.vector_3_float64)
    rotated = sc.Variable(value=[-1, -2, 3],
                          unit=sc.units.m,
                          dtype=sc.dtype.vector_3_float64)
    assert rot * vec == rotated


def test_variable_0D_matrix_from_numpy():
    var = sc.Variable(value=np.arange(9).reshape(3, 3),
                      unit=sc.units.m,
                      dtype=sc.dtype.matrix_3_float64)
    np.testing.assert_array_equal(var.value, np.arange(9).reshape(3, 3))
    assert var.dtype == sc.dtype.matrix_3_float64
    assert var.unit == sc.units.m


def test_variable_1D_matrix_from_numpy():
    data = np.array([
        np.arange(9.0).reshape(3, 3),
        np.arange(5.0, 14.0).reshape(3, 3),
        np.arange(1.0, 10.0).reshape(3, 3)
    ])
    var = sc.Variable(['tof'],
                      values=data,
                      unit=sc.units.us,
                      dtype=sc.dtype.matrix_3_float64)
    assert len(var.values) == 3
    np.testing.assert_array_equal(var.values[0],
                                  [[0, 1, 2], [3, 4, 5], [6, 7, 8]])
    np.testing.assert_array_equal(var.values[1],
                                  [[5, 6, 7], [8, 9, 10], [11, 12, 13]])
    np.testing.assert_array_equal(var.values[2],
                                  [[1, 2, 3], [4, 5, 6], [7, 8, 9]])
    assert var.dims == ['tof']
    assert var.dtype == sc.dtype.matrix_3_float64
    assert var.unit == sc.units.us
