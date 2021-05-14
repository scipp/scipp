# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc


def test_variable_0D_vector_3_float64_from_list():
    var = sc.vector(value=[1, 2, 3], unit=sc.units.m)
    np.testing.assert_array_equal(var.value, [1, 2, 3])
    assert var.dtype == sc.dtype.vector_3_float64
    assert var.unit == sc.units.m


def test_variable_0D_vector_3_float64_from_numpy():
    var = sc.vector(value=np.array([1, 2, 3]), unit=sc.units.m)
    np.testing.assert_array_equal(var.value, [1, 2, 3])
    assert var.dtype == sc.dtype.vector_3_float64
    assert var.unit == sc.units.m


def test_variable_1D_vector_3_float64_from_list():
    var = sc.vectors(dims=['x'],
                     values=[[1, 2, 3], [4, 5, 6]],
                     unit=sc.units.m)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == ['x']
    assert var.dtype == sc.dtype.vector_3_float64
    assert var.unit == sc.units.m


def test_variable_1D_vector_3_float64_from_numpy():
    var = sc.vectors(dims=['x'],
                     values=np.array([[1, 2, 3], [4, 5, 6]]),
                     unit=sc.units.m)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == ['x']
    assert var.dtype == sc.dtype.vector_3_float64
    assert var.unit == sc.units.m


def test_set_vector_value():
    var = sc.vector(value=np.array([1, 2, 3]), unit=sc.units.m)
    value = np.array([3, 2, 1])
    ref = sc.vector(value=value, unit=sc.units.m)
    var.value = value
    assert sc.identical(var, ref)


def test_set_vectors_values():
    var = sc.vectors(dims=['x'],
                     values=np.array([[1, 2, 3], [4, 5, 6]]),
                     unit=sc.units.m)
    values = np.array([[6, 5, 4], [3, 2, 1]])
    ref = sc.vectors(dims=['x'], values=values, unit=sc.units.m)
    var.values = values
    assert sc.identical(var, ref)


def test_matrix_from_quat_coeffs_list():
    sc.geometry.rotation_matrix_from_quaternion_coeffs([1, 2, 3, 4])


def test_matrix_from_quat_coeffs_numpy():
    sc.geometry.rotation_matrix_from_quaternion_coeffs(np.arange(4))


def test_variable_0D_matrix():
    # Use known rotation (180 deg around z) to check correct construction
    rot = sc.matrix(value=sc.geometry.rotation_matrix_from_quaternion_coeffs(
        [0, 0, 1, 0]),
                    unit=sc.units.one)
    vec = sc.vector(value=[1, 2, 3], unit=sc.units.m)
    rotated = sc.vector(value=[-1, -2, 3], unit=sc.units.m)
    assert sc.identical(rot * vec, rotated)


def test_variable_0D_matrix_from_numpy():
    var = sc.matrix(value=np.arange(9).reshape(3, 3), unit=sc.units.m)
    np.testing.assert_array_equal(var.value, np.arange(9).reshape(3, 3))
    assert var.dtype == sc.dtype.matrix_3_float64
    assert var.unit == sc.units.m


def test_variable_1D_matrix_from_numpy():
    data = np.array([
        np.arange(9.0).reshape(3, 3),
        np.arange(5.0, 14.0).reshape(3, 3),
        np.arange(1.0, 10.0).reshape(3, 3)
    ])
    var = sc.matrices(dims=['x'], values=data, unit=sc.units.us)
    assert len(var.values) == 3
    np.testing.assert_array_equal(var.values[0],
                                  [[0, 1, 2], [3, 4, 5], [6, 7, 8]])
    np.testing.assert_array_equal(var.values[1],
                                  [[5, 6, 7], [8, 9, 10], [11, 12, 13]])
    np.testing.assert_array_equal(var.values[2],
                                  [[1, 2, 3], [4, 5, 6], [7, 8, 9]])
    assert var.dims == ['x']
    assert var.dtype == sc.dtype.matrix_3_float64
    assert var.unit == sc.units.us


def test_set_matrix_value():
    var = sc.matrix(value=np.arange(9).reshape(3, 3), unit=sc.units.m)
    value = np.arange(9, 18).reshape(3, 3)
    ref = sc.matrix(value=value, unit=sc.units.m)
    var.value = value
    assert sc.identical(var, ref)


def test_set_matrices_values():
    var = sc.matrices(dims=['x'],
                      values=np.arange(18).reshape(2, 3, 3),
                      unit=sc.units.m)
    values = np.arange(18, 36).reshape(2, 3, 3)
    ref = sc.matrices(dims=['x'], values=values, unit=sc.units.m)
    var.values = values
    assert sc.identical(var, ref)
