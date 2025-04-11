# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np
import pytest

import scipp as sc


def test_variable_0D_vector3_from_list() -> None:
    var = sc.vector(value=[1, 2, 3], unit=sc.units.m)
    np.testing.assert_array_equal(var.value, [1, 2, 3])
    assert var.dtype == sc.DType.vector3
    assert var.unit == sc.units.m


def test_variable_0D_vector3_from_numpy() -> None:
    var = sc.vector(value=np.array([1, 2, 3]), unit=sc.units.m)
    np.testing.assert_array_equal(var.value, [1, 2, 3])
    assert var.dtype == sc.DType.vector3
    assert var.unit == sc.units.m


def test_variable_1D_vector3_from_list() -> None:
    var = sc.vectors(dims=['x'], values=[[1, 2, 3], [4, 5, 6]], unit=sc.units.m)
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == ('x',)
    assert var.dtype == sc.DType.vector3
    assert var.unit == sc.units.m


def test_variable_1D_vector3_from_numpy() -> None:
    var = sc.vectors(
        dims=['x'], values=np.array([[1, 2, 3], [4, 5, 6]]), unit=sc.units.m
    )
    assert len(var.values) == 2
    np.testing.assert_array_equal(var.values[0], [1, 2, 3])
    np.testing.assert_array_equal(var.values[1], [4, 5, 6])
    assert var.dims == ('x',)
    assert var.dtype == sc.DType.vector3
    assert var.unit == sc.units.m


def test_set_vector_value() -> None:
    var = sc.vector(value=np.array([1, 2, 3]), unit=sc.units.m)
    value = np.array([3, 2, 1])
    ref = sc.vector(value=value, unit=sc.units.m)
    var.value = value
    assert sc.identical(var, ref)


def test_set_vectors_values() -> None:
    var = sc.vectors(
        dims=['x'], values=np.array([[1, 2, 3], [4, 5, 6]]), unit=sc.units.m
    )
    values = np.array([[6, 5, 4], [3, 2, 1]])
    ref = sc.vectors(dims=['x'], values=values, unit=sc.units.m)
    var.values = values
    assert sc.identical(var, ref)


def test_vector_elements() -> None:
    var = sc.vectors(
        dims=['x'], values=np.array([[1, 2, 3], [4, 5, 6]]), unit=sc.units.m
    )
    assert sc.identical(
        var.fields.x, sc.array(dims=['x'], values=np.array([1.0, 4.0]), unit=sc.units.m)
    )
    assert sc.identical(
        var.fields.y, sc.array(dims=['x'], values=np.array([2.0, 5.0]), unit=sc.units.m)
    )
    assert sc.identical(
        var.fields.z, sc.array(dims=['x'], values=np.array([3.0, 6.0]), unit=sc.units.m)
    )


def test_vector_elements_setter() -> None:
    var = sc.vectors(
        dims=['x'], values=np.array([[1, 2, 3], [4, 5, 6]]), unit=sc.units.m
    )

    var.fields.x = sc.array(dims=['x'], values=np.array([7.0, 8.0]), unit=sc.units.m)
    expected = sc.vectors(
        dims=['x'], values=np.array([[7, 2, 3], [8, 5, 6]]), unit=sc.units.m
    )
    assert sc.identical(var, expected)

    var.fields.y = sc.array(dims=['x'], values=np.array([7.0, 8.0]), unit=sc.units.m)
    assert not sc.identical(var, expected)
    expected = sc.vectors(
        dims=['x'], values=np.array([[7, 7, 3], [8, 8, 6]]), unit=sc.units.m
    )
    assert sc.identical(var, expected)

    var.fields.z = sc.array(dims=['x'], values=np.array([7.0, 8.0]), unit=sc.units.m)
    assert not sc.identical(var, expected)
    expected = sc.vectors(
        dims=['x'], values=np.array([[7, 7, 7], [8, 8, 8]]), unit=sc.units.m
    )
    assert sc.identical(var, expected)


def test_vector_readonly_elements() -> None:
    vec = sc.vector(value=[1, 2, 3], unit=sc.units.m)
    var = sc.broadcast(vec, dims=['x'], shape=[2])
    with pytest.raises(sc.VariableError):
        var['x', 0] = vec + vec
    with pytest.raises(sc.VariableError):
        var.fields.x = 1.1 * sc.units.m
    assert not var.values.flags['WRITEABLE']
    assert not var.fields.x.values.flags['WRITEABLE']


def test_elements_binned() -> None:
    data = sc.array(dims=['x'], values=[1, 2, 3, 4])
    var = sc.bins(dim='x', data=data)
    assert var.fields is None

    data = sc.vectors(dims=['x'], values=np.arange(6).reshape(2, 3))
    var = sc.bins(dim='x', data=data)
    assert var.fields is not None
    y = sc.bins(dim='x', data=sc.array(dims=['x'], values=[1.0, 4.0]))
    assert sc.identical(var.fields.y, y)


def test_vectors_None_unit_yields_variable_with_None_unit() -> None:
    assert sc.vectors(dims=['x'], values=np.ones(shape=(2, 3)), unit=None).unit is None


def test_vector_default_unit_is_dimensionless() -> None:
    var = sc.vector(value=np.ones(shape=(3,)))
    assert var.unit == sc.units.one


def test_vectors_default_unit_is_dimensionless() -> None:
    var = sc.vectors(dims=['x'], values=np.ones(shape=(2, 3)))
    assert var.unit == sc.units.one
