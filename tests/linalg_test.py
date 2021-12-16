# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import scipp as sc
import pytest


def test_variable_0D_vector3_from_list():
    var = sc.vector(value=[1, 2, 3], unit=sc.units.m)
    np.testing.assert_array_equal(var.value, [1, 2, 3])
    assert var.dtype == sc.dtype.vector3
    assert var.unit == sc.units.m


def test_variable_0D_vector3_from_numpy():
    var = sc.vector(value=np.array([1, 2, 3]), unit=sc.units.m)
    np.testing.assert_array_equal(var.value, [1, 2, 3])
    assert var.dtype == sc.dtype.vector3
    assert var.unit == sc.units.m


def test_variable_1D_vector3_from_list():
    var = sc.vectors(dims=['x'], values=[[1, 2, 3], [4, 5, 6]], unit=sc.units.m)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == ['x']
    assert var.dtype == sc.dtype.vector3
    assert var.unit == sc.units.m


def test_variable_1D_vector3_from_numpy():
    var = sc.vectors(dims=['x'],
                     values=np.array([[1, 2, 3], [4, 5, 6]]),
                     unit=sc.units.m)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == ['x']
    assert var.dtype == sc.dtype.vector3
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


def test_vector_elements():
    var = sc.vectors(dims=['x'],
                     values=np.array([[1, 2, 3], [4, 5, 6]]),
                     unit=sc.units.m)
    assert sc.identical(
        var.fields.x, sc.array(dims=['x'], values=np.array([1., 4.]), unit=sc.units.m))
    assert sc.identical(
        var.fields.y, sc.array(dims=['x'], values=np.array([2., 5.]), unit=sc.units.m))
    assert sc.identical(
        var.fields.z, sc.array(dims=['x'], values=np.array([3., 6.]), unit=sc.units.m))


def test_vector_elements_setter():
    var = sc.vectors(dims=['x'],
                     values=np.array([[1, 2, 3], [4, 5, 6]]),
                     unit=sc.units.m)

    var.fields.x = sc.array(dims=['x'], values=np.array([7., 8.]), unit=sc.units.m)
    expected = sc.vectors(dims=['x'],
                          values=np.array([[7, 2, 3], [8, 5, 6]]),
                          unit=sc.units.m)
    assert sc.identical(var, expected)

    var.fields.y = sc.array(dims=['x'], values=np.array([7., 8.]), unit=sc.units.m)
    assert not sc.identical(var, expected)
    expected = sc.vectors(dims=['x'],
                          values=np.array([[7, 7, 3], [8, 8, 6]]),
                          unit=sc.units.m)
    assert sc.identical(var, expected)

    var.fields.z = sc.array(dims=['x'], values=np.array([7., 8.]), unit=sc.units.m)
    assert not sc.identical(var, expected)
    expected = sc.vectors(dims=['x'],
                          values=np.array([[7, 7, 7], [8, 8, 8]]),
                          unit=sc.units.m)
    assert sc.identical(var, expected)


def test_vector_readonly_elements():
    vec = sc.vector(value=[1, 2, 3], unit=sc.units.m)
    var = sc.broadcast(vec, dims=['x'], shape=[2])
    with pytest.raises(sc.VariableError):
        var['x', 0] = vec + vec
    with pytest.raises(sc.VariableError):
        var.fields.x = 1.1 * sc.units.m
    assert not var.values.flags['WRITEABLE']
    assert not var.fields.x.values.flags['WRITEABLE']


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
    assert var.dtype == sc.dtype.linear_transform3
    assert var.unit == sc.units.m


def test_variable_1D_matrix_from_numpy():
    data = np.array([
        np.arange(9.0).reshape(3, 3),
        np.arange(5.0, 14.0).reshape(3, 3),
        np.arange(1.0, 10.0).reshape(3, 3)
    ])
    var = sc.matrices(dims=['x'], values=data, unit=sc.units.us)
    assert len(var.values) == 3
    np.testing.assert_array_equal(var.values[0], [[0, 1, 2], [3, 4, 5], [6, 7, 8]])
    np.testing.assert_array_equal(var.values[1], [[5, 6, 7], [8, 9, 10], [11, 12, 13]])
    np.testing.assert_array_equal(var.values[2], [[1, 2, 3], [4, 5, 6], [7, 8, 9]])
    assert var.dims == ['x']
    assert var.dtype == sc.dtype.linear_transform3
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


def test_matrix_elements():
    m = sc.matrices(dims=['x'], values=np.arange(18).reshape(2, 3, 3), unit=sc.units.m)
    f = m.fields
    elems = [f.xx, f.xy, f.xz, f.yx, f.yy, f.yz, f.zx, f.zy, f.zz]
    offsets = range(9)
    for elem, offset in zip(elems, offsets):
        assert sc.identical(
            elem,
            sc.array(dims=['x'], values=np.array([0., 9.]) + offset, unit=sc.units.m))


def test_matrix_elements_setter():
    values = np.arange(18).reshape(2, 3, 3)
    m = sc.matrices(dims=['x'], values=values, unit=sc.units.m)

    m.fields.xx = sc.array(dims=['x'], values=np.array([1., 1.]), unit=sc.units.m)
    expected = sc.matrices(dims=['x'], values=values, unit=sc.units.m)
    assert not sc.identical(m, expected)
    values[:, 0, 0] = 1
    expected = sc.matrices(dims=['x'], values=values, unit=sc.units.m)
    assert sc.identical(m, expected)

    m.fields.yz = sc.array(dims=['x'], values=np.array([1., 1.]), unit=sc.units.m)
    assert not sc.identical(m, expected)
    values[:, 1, 2] = 1
    expected = sc.matrices(dims=['x'], values=values, unit=sc.units.m)
    assert sc.identical(m, expected)


def test_elements_binned():
    data = sc.array(dims=['x'], values=[1, 2, 3, 4])
    var = sc.bins(dim='x', data=data)
    assert var.fields is None

    data = sc.vectors(dims=['x'], values=np.arange(6).reshape(2, 3))
    var = sc.bins(dim='x', data=data)
    assert var.fields is not None
    y = sc.bins(dim='x', data=sc.array(dims=['x'], values=[1., 4.]))
    assert sc.identical(var.fields.y, y)

    data = sc.matrices(dims=['x'], values=np.arange(18).reshape(2, 3, 3))
    var = sc.bins(dim='x', data=data)
    assert var.fields is not None
    yz = sc.bins(dim='x', data=sc.array(dims=['x'], values=[5., 14.]))
    assert sc.identical(var.fields.yz, yz)
