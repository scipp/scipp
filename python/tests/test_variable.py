# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock

import numpy as np
import pytest

import scipp as sc

from .common import assert_export


def make_variables():
    data = np.arange(1, 4, dtype=float)
    a = sc.Variable(dims=['x'], values=data)
    b = sc.Variable(dims=['x'], values=data)
    a_slice = a['x', :]
    b_slice = b['x', :]
    return a, b, a_slice, b_slice, data


def test_create_default():
    var = sc.Variable()
    assert var.dims == []
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.dimensionless
    assert var.value == 0.0


def test_create_default_dtype():
    var = sc.Variable(dims=['x'], shape=[4])
    assert var.dtype == sc.dtype.float64


def test_create_with_dtype():
    var = sc.Variable(dims=['x'], shape=[2], dtype=sc.dtype.float32)
    assert var.dtype == sc.dtype.float32


def test_create_with_numpy_dtype():
    var = sc.Variable(dims=['x'], shape=[2], dtype=np.dtype(np.float32))
    assert var.dtype == sc.dtype.float32


def test_create_with_variances():
    assert sc.Variable(dims=['x'], shape=[2]).variances is None
    assert sc.Variable(dims=['x'], shape=[2],
                       variances=False).variances is None
    assert sc.Variable(dims=['x'], shape=[2],
                       variances=True).variances is not None


def test_create_with_shape_and_variances():
    # If no values are given, variances must be Bool, cannot pass array.
    with pytest.raises(TypeError):
        sc.Variable(dims=['x'], shape=[2], variances=np.arange(2))


def test_create_events():
    var = sc.Variable(dims=['x'], shape=[4], dtype=sc.dtype.event_list_float64)
    assert var.dtype == sc.dtype.event_list_float64
    assert len(var.values) == 4
    for vals in var.values:
        assert len(vals) == 0


def test_create_from_numpy_1d():
    var = sc.Variable(dims=['x'], values=np.arange(4.0))
    assert var.dtype == sc.dtype.float64
    np.testing.assert_array_equal(var.values, np.arange(4))


def test_create_from_numpy_1d_bool():
    var = sc.Variable(dims=['x'], values=np.array([True, False, True]))
    assert var.dtype == sc.dtype.bool
    np.testing.assert_array_equal(var.values, np.array([True, False, True]))


def test_create_with_variances_from_numpy_1d():
    var = sc.Variable(dims=['x'],
                      values=np.arange(4.0),
                      variances=np.arange(4.0, 8.0))
    assert var.dtype == sc.dtype.float64
    np.testing.assert_array_equal(var.values, np.arange(4))
    np.testing.assert_array_equal(var.variances, np.arange(4, 8))


def test_create_scalar():
    var = sc.Variable(1.2)
    assert var.value == 1.2
    assert var.dims == []
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.dimensionless


def test_create_scalar_Dataset():
    dataset = sc.Dataset({'a': sc.Variable(dims=['x'], values=np.arange(4.0))})
    var = sc.Variable(dataset)
    assert var.value == dataset
    assert var.dims == []
    assert var.dtype == sc.dtype.Dataset
    assert var.unit == sc.units.dimensionless


def test_create_scalar_quantity():
    var = sc.Variable(1.2, unit=sc.units.m)
    assert var.value == 1.2
    assert var.dims == []
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.m


def test_create_via_unit():
    expected = sc.Variable(1.2, unit=sc.units.m)
    var = 1.2 * sc.units.m
    assert var == expected


def test_create_1D_string():
    var = sc.Variable(dims=['row'], values=['a', 'bb'], unit=sc.units.m)
    assert len(var.values) == 2
    assert var.values[0] == 'a'
    assert var.values[1] == 'bb'
    assert var.dims == ['row']
    assert var.dtype == sc.dtype.string
    assert var.unit == sc.units.m


def test_create_1D_vector_3_float64():
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


def test_create_2D_inner_size_3():
    var = sc.Variable(dims=['x', 'y'],
                      values=np.arange(6.0).reshape(2, 3),
                      unit=sc.units.m)
    assert var.shape == [2, 3]
    np.testing.assert_array_equal(var.values[0], [0, 1, 2])
    np.testing.assert_array_equal(var.values[1], [3, 4, 5])
    assert var.dims == ['x', 'y']
    assert var.dtype == sc.dtype.float64
    assert var.unit == sc.units.m


def test_astype():
    var = sc.Variable(dims=['x'],
                      values=np.array([1, 2, 3, 4], dtype=np.int64))
    assert var.dtype == sc.dtype.int64

    var_as_float = var.astype(sc.dtype.float32)
    assert var_as_float.dtype == sc.dtype.float32


def test_astype_bad_conversion():
    var = sc.Variable(dims=['x'],
                      values=np.array([1, 2, 3, 4], dtype=np.int64))
    assert var.dtype == sc.dtype.int64

    with pytest.raises(RuntimeError):
        var.astype(sc.dtype.string)


def test_operation_with_scalar_quantity():
    reference = sc.Variable(dims=['x'], values=np.arange(4.0) * 1.5)
    reference.unit = sc.units.kg

    var = sc.Variable(dims=['x'], values=np.arange(4.0))
    var *= sc.Variable(1.5, unit=sc.units.kg)
    assert var == reference


def test_0D_scalar_access():
    var = sc.Variable()
    assert var.value == 0.0
    var.value = 1.2
    assert var.value == 1.2
    assert var.values.shape == ()
    assert var.values == 1.2


def test_0D_scalar_string():
    var = sc.Variable(value='a')
    assert var.value == 'a'
    var.value = 'b'
    assert var == sc.Variable(value='b')


def test_1D_scalar_access_fail():
    var = sc.Variable(dims=['x'], shape=(1, ))
    with pytest.raises(RuntimeError):
        assert var.value == 0.0
    with pytest.raises(RuntimeError):
        var.value = 1.2


def test_1D_access_shape_mismatch_fail():
    var = sc.Variable(dims=['x'], shape=(2, ))
    with pytest.raises(RuntimeError):
        var.values = 1.2


def test_1D_access():
    var = sc.Variable(dims=['x'], shape=(2, ))
    assert len(var.values) == 2
    assert var.values.shape == (2, )
    var.values[1] = 1.2
    assert var.values[1] == 1.2


def test_1D_set_from_list():
    var = sc.Variable(dims=['x'], shape=(2, ))
    var.values = [1.0, 2.0]
    assert var == sc.Variable(dims=['x'], values=[1.0, 2.0])


def test_1D_string():
    var = sc.Variable(dims=['x'], values=['a', 'b'])
    assert len(var.values) == 2
    assert var.values[0] == 'a'
    assert var.values[1] == 'b'
    var.values = ['c', 'd']
    assert var == sc.Variable(dims=['x'], values=['c', 'd'])


def test_1D_converting():
    var = sc.Variable(dims=['x'], values=[1, 2])
    var.values = [3.3, 4.6]
    # floats get truncated
    assert var == sc.Variable(dims=['x'], values=[3, 4])


def test_1D_dataset():
    var = sc.Variable(dims=['x'], shape=(2, ), dtype=sc.dtype.Dataset)
    d1 = sc.Dataset({'a': 1.5 * sc.units.m})
    d2 = sc.Dataset({'a': 2.5 * sc.units.m})
    var.values = [d1, d2]
    assert var.values[0] == d1
    assert var.values[1] == d2


def test_1D_access_bad_shape_fail():
    var = sc.Variable(dims=['x'], shape=(2, ))
    with pytest.raises(RuntimeError):
        var.values = np.arange(3)


def test_2D_access():
    var = sc.Variable(dims=['x', 'y'], shape=(2, 3))
    assert var.values.shape == (2, 3)
    assert len(var.values) == 2
    assert len(var.values[0]) == 3
    var.values[1] = 1.2  # numpy assigns to all elements in "slice"
    var.values[1][2] = 2.2
    assert var.values[1][0] == 1.2
    assert var.values[1][1] == 1.2
    assert var.values[1][2] == 2.2


def test_2D_access_bad_shape_fail():
    var = sc.Variable(dims=['x', 'y'], shape=(2, 3))
    with pytest.raises(RuntimeError):
        var.values = np.ones(shape=(3, 2))


def test_2D_access_variances():
    var = sc.Variable(dims=['x', 'y'], shape=(2, 3), variances=True)
    assert var.values.shape == (2, 3)
    assert var.variances.shape == (2, 3)
    var.values[1] = 1.2
    assert np.array_equal(var.variances, np.zeros(shape=(2, 3)))
    var.variances = np.ones(shape=(2, 3))
    assert np.array_equal(var.variances, np.ones(shape=(2, 3)))


def test_events_slice():
    var = sc.Variable(dims=['x'], shape=[4], dtype=sc.dtype.event_list_float64)
    vals0 = var['x', 0].values
    assert len(vals0) == 0
    vals0.append(1.2)
    assert len(var['x', 0].values) == 1


def test_events_setitem():
    var = sc.Variable(dims=['x'], shape=[4], dtype=sc.dtype.event_list_float64)
    # __setitem__ of vector
    var['x', 0].values = np.arange(4)
    assert len(var['x', 0].values) == 4
    # __setitem__ of span
    var.values[1] = np.arange(3)
    assert len(var['x', 1].values) == 3
    # __setitem__ of ElementArrayView
    var['x', :].values[2] = np.arange(2)
    assert len(var['x', 2].values) == 2


def test_events_setitem_events_fail():
    var = sc.Variable(dims=['x'], shape=[4], dtype=sc.dtype.event_list_float64)
    with pytest.raises(RuntimeError):
        var.values = np.arange(3)


def test_events_setitem_shape_fail():
    var = sc.Variable(dims=['x'], shape=[4], dtype=sc.dtype.event_list_float64)
    with pytest.raises(RuntimeError):
        var['x', 0].values = np.ones(shape=(3, 2))


def test_events_setitem_float():
    var = sc.Variable(dims=['x'], shape=[4], dtype=sc.dtype.event_list_float32)
    var['x', 0].values = np.arange(4)
    assert len(var['x', 0].values) == 4


def test_events_setitem_int64_t():
    var = sc.Variable(dims=['x'], shape=[4], dtype=sc.dtype.event_list_int64)
    var['x', 0].values = np.arange(4)
    assert len(var['x', 0].values) == 4


def test_create_dtype():
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.int64))
    assert var.dtype == sc.dtype.int64
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.int32))
    assert var.dtype == sc.dtype.int32
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.float64))
    assert var.dtype == sc.dtype.float64
    var = sc.Variable(dims=['x'], values=np.arange(4).astype(np.float32))
    assert var.dtype == sc.dtype.float32
    var = sc.Variable(dims=['x'], shape=(4, ), dtype=np.dtype(np.float64))
    assert var.dtype == sc.dtype.float64
    var = sc.Variable(dims=['x'], shape=(4, ), dtype=np.dtype(np.float32))
    assert var.dtype == sc.dtype.float32
    var = sc.Variable(dims=['x'], shape=(4, ), dtype=np.dtype(np.int64))
    assert var.dtype == sc.dtype.int64
    var = sc.Variable(dims=['x'], shape=(4, ), dtype=np.dtype(np.int32))
    assert var.dtype == sc.dtype.int32


def test_get_slice():
    var = sc.Variable(dims=['x', 'y'], values=np.arange(0, 8).reshape(2, 4))
    var_slice = var['x', 1:2]
    assert var_slice == sc.Variable(dims=['x', 'y'],
                                    values=np.arange(4, 8).reshape(1, 4))


def test_slicing():
    var = sc.Variable(dims=['x'], values=np.arange(0, 3))
    var_slice = var[('x', slice(0, 2))]
    assert isinstance(var_slice, sc.VariableView)
    assert len(var_slice.values) == 2
    assert np.array_equal(var_slice.values, np.array([0, 1]))


def test_iadd():
    expected = sc.Variable(2.2)
    a = sc.Variable(1.2)
    b = a
    a += 1.0
    assert a == expected
    assert b == expected
    # This extra check is important: It can happen that an implementation of,
    # e.g., __iadd__ does an in-place modification, updating `b`, but then the
    # return value is assigned to `a`, which could break the connection unless
    # the correct Python object is returned.
    a += 1.0
    assert a == b


def test_isub():
    expected = sc.Variable(2.2 - 1.0)
    a = sc.Variable(2.2)
    b = a
    a -= 1.0
    assert a == expected
    assert b == expected
    a -= 1.0
    assert a == b


def test_imul():
    expected = sc.Variable(2.4)
    a = sc.Variable(1.2)
    b = a
    a *= 2.0
    assert a == expected
    assert b == expected
    a *= 2.0
    assert a == b


def test_idiv():
    expected = sc.Variable(1.2)
    a = sc.Variable(2.4)
    b = a
    a /= 2.0
    assert a == expected
    assert b == expected
    a /= 2.0
    assert a == b


def test_iand():
    expected = sc.Variable(False)
    a = sc.Variable(True)
    b = a
    a &= sc.Variable(False)
    assert a == expected
    assert b == expected
    a |= sc.Variable(True)
    assert a == b


def test_ior():
    expected = sc.Variable(True)
    a = sc.Variable(False)
    b = a
    a |= sc.Variable(True)
    assert a == expected
    assert b == expected
    a &= sc.Variable(False)
    assert a == b


def test_ixor():
    expected = sc.Variable(True)
    a = sc.Variable(False)
    b = a
    a ^= sc.Variable(True)
    assert a == expected
    assert b == expected
    a ^= sc.Variable(True)
    assert a == b


def test_binary_plus():
    a, b, a_slice, b_slice, data = make_variables()
    c = a + b
    assert np.array_equal(c.values, data + data)
    c = a + 2.0
    assert np.array_equal(c.values, data + 2.0)
    c = a + b_slice
    assert np.array_equal(c.values, data + data)
    c += b
    assert np.array_equal(c.values, data + data + data)
    c += b_slice
    assert np.array_equal(c.values, data + data + data + data)
    c = 3.5 + c
    assert np.array_equal(c.values, data + data + data + data + 3.5)


def test_binary_minus():
    a, b, a_slice, b_slice, data = make_variables()
    c = a - b
    assert np.array_equal(c.values, data - data)
    c = a - 2.0
    assert np.array_equal(c.values, data - 2.0)
    c = a - b_slice
    assert np.array_equal(c.values, data - data)
    c -= b
    assert np.array_equal(c.values, data - data - data)
    c -= b_slice
    assert np.array_equal(c.values, data - data - data - data)
    c = 3.5 - c
    assert np.array_equal(c.values, 3.5 - data + data + data + data)


def test_binary_multiply():
    a, b, a_slice, b_slice, data = make_variables()
    c = a * b
    assert np.array_equal(c.values, data * data)
    c = a * 2.0
    assert np.array_equal(c.values, data * 2.0)
    c = a * b_slice
    assert np.array_equal(c.values, data * data)
    c *= b
    assert np.array_equal(c.values, data * data * data)
    c *= b_slice
    assert np.array_equal(c.values, data * data * data * data)
    c = 3.5 * c
    assert np.array_equal(c.values, data * data * data * data * 3.5)


def test_binary_divide():
    a, b, a_slice, b_slice, data = make_variables()
    c = a / b
    assert np.array_equal(c.values, data / data)
    c = a / 2.0
    assert np.array_equal(c.values, data / 2.0)
    c = a / b_slice
    assert np.array_equal(c.values, data / data)
    c /= b
    assert np.array_equal(c.values, data / data / data)
    c /= b_slice
    assert np.array_equal(c.values, data / data / data / data)
    c = 2.0 / a
    assert np.array_equal(c.values, 2.0 / data)


def test_in_place_binary_or():
    a = sc.Variable(False)
    b = sc.Variable(True)
    a |= b
    assert a == sc.Variable(True)

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a |= b
    assert a == sc.Variable(dims=['x'],
                            values=np.array([False, True, True, True]))


def test_binary_or():
    a = sc.Variable(False)
    b = sc.Variable(True)
    assert (a | b) == sc.Variable(True)

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert (a | b) == sc.Variable(dims=['x'],
                                  values=np.array([False, True, True, True]))


def test_in_place_binary_and():
    a = sc.Variable(False)
    b = sc.Variable(True)
    a &= b
    assert a == sc.Variable(False)

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a &= b
    assert a == sc.Variable(dims=['x'],
                            values=np.array([False, False, False, True]))


def test_binary_and():
    a = sc.Variable(False)
    b = sc.Variable(True)
    assert (a & b) == sc.Variable(False)

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert (a & b) == sc.Variable(dims=['x'],
                                  values=np.array([False, False, False, True]))


def test_in_place_binary_xor():
    a = sc.Variable(False)
    b = sc.Variable(True)
    a ^= b
    assert a == sc.Variable(True)

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a ^= b
    assert a == sc.Variable(dims=['x'],
                            values=np.array([False, True, True, False]))


def test_binary_xor():
    a = sc.Variable(False)
    b = sc.Variable(True)
    assert (a ^ b) == sc.Variable(True)

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert (a ^ b) == sc.Variable(dims=['x'],
                                  values=np.array([False, True, True, False]))


def test_in_place_binary_with_scalar():
    v = sc.Variable(dims=['x'], values=[10])
    copy = v.copy()

    v += 2
    v *= 2
    v -= 4
    v /= 2
    assert v == copy


def test_binary_equal():
    a, b, a_slice, b_slice, data = make_variables()
    assert a == b
    assert a == a_slice
    assert a_slice == b_slice
    assert b == a
    assert b_slice == a
    assert b_slice == a_slice


def test_binary_not_equal():
    a, b, a_slice, b_slice, data = make_variables()
    c = a + b
    assert a != c
    assert a_slice != c
    assert c != a
    assert c != a_slice


def test_abs():
    assert_export(sc.abs, sc.Variable())


def test_abs_out():
    var = sc.Variable()
    assert_export(sc.abs, var, out=var)


def test_dot():
    assert_export(sc.dot, sc.Variable(), sc.Variable())


def test_concatenate():
    assert_export(sc.concatenate, sc.Variable(), sc.Variable(), 'x')


def test_mean():
    assert_export(sc.mean, sc.Variable(), 'x')


def test_mean_in_place():
    var = sc.Variable()
    assert_export(sc.mean, sc.Variable(), 'x', var)


def test_norm():
    assert_export(sc.norm, sc.Variable())


def test_sqrt():
    assert_export(sc.sqrt, sc.Variable())


def test_sqrt_out():
    var = sc.Variable()
    assert_export(sc.sqrt, var, var)


def test_values_variances():
    assert_export(sc.values, sc.Variable())
    assert_export(sc.variances, sc.Variable())


def test_sum():
    var = sc.Variable(dims=['x', 'y'],
                      values=np.array([[0.1, 0.3], [0.2, 0.6]]),
                      unit=sc.units.m)
    expected = sc.Variable(dims=['x'],
                           values=np.array([0.4, 0.8]),
                           unit=sc.units.m)
    assert sc.sum(var, 'y') == expected


def test_sum_in_place():
    var = sc.Variable(dims=['x', 'y'],
                      values=np.array([[0.1, 0.3], [0.2, 0.6]]),
                      unit=sc.units.m)
    out_var = sc.Variable(dims=['x'],
                          values=np.array([0.0, 0.0]),
                          unit=sc.units.m)
    expected = sc.Variable(dims=['x'],
                           values=np.array([0.4, 0.8]),
                           unit=sc.units.m)
    out_view = sc.sum(var, 'y', out=out_var)
    assert out_var == expected
    assert out_view == expected


def test_variance_acess():
    v = sc.Variable()
    assert v.variance is None
    assert v.variances is None


def test_set_variance():
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sc.Variable(dims=['x', 'y'], values=values)
    expected = sc.Variable(dims=['x', 'y'], values=values, variances=variances)

    assert var.variances is None
    assert var != expected

    var.variances = variances

    assert var.variances is not None
    assert var == expected


def test_copy_variance():
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sc.Variable(dims=['x', 'y'], values=values)
    expected = sc.Variable(dims=['x', 'y'], values=values, variances=variances)

    assert var.variances is None
    assert var != expected

    var.variances = expected.variances

    assert var.variances is not None
    assert var == expected


def test_remove_variance():
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sc.Variable(dims=['x', 'y'], values=values, variances=variances)
    expected = sc.Variable(dims=['x', 'y'], values=values)
    assert var.variances is not None
    var.variances = None
    assert var.variances is None
    assert var == expected


def test_set_variance_convert_dtype():
    values = np.random.rand(2, 3)
    variances = np.arange(6).reshape(2, 3)
    assert variances.dtype == np.int
    var = sc.Variable(dims=['x', 'y'], values=values)
    expected = sc.Variable(dims=['x', 'y'], values=values, variances=variances)

    assert var.variances is None
    assert var != expected

    var.variances = variances

    assert var.variances is not None
    assert var == expected


def test_sum_mean():
    var = sc.Variable(dims=['x'], values=np.arange(5, dtype=np.int64))
    assert sc.sum(var, 'x') == sc.Variable(10)
    var = sc.Variable(dims=['x'], values=np.arange(6, dtype=np.int64))
    assert sc.mean(var, 'x') == sc.Variable(2.5)


def test_make_variable_from_unit_scalar_mult_div():
    var = sc.Variable()
    var.unit = sc.units.m
    assert var == 0.0 * sc.units.m
    var.unit = sc.units.m**(-1)
    assert var == 0.0 / sc.units.m

    var = sc.Variable(value=np.float32())
    var.unit = sc.units.m
    assert var == np.float32(0.0) * sc.units.m
    var.unit = sc.units.m**(-1)
    assert var == np.float32(0.0) / sc.units.m


def test_construct_0d_numpy():
    v = sc.Variable(dims=['x'], values=np.array([0]), dtype=np.float32)
    var = sc.Variable(v['x', 0])
    assert var == sc.Variable(np.float32())

    v = sc.Variable(dims=['x'], values=np.array([0]), dtype=np.float32)
    var = sc.Variable(v['x', 0])
    var.unit = sc.units.m
    assert var == np.float32(0.0) * sc.units.m
    var.unit = sc.units.m**(-1)
    assert var == np.float32(0.0) / sc.units.m


def test_construct_0d_native_python_types():
    assert sc.Variable(2).dtype == sc.dtype.int64
    assert sc.Variable(2.0).dtype == sc.dtype.float64
    assert sc.Variable(True).dtype == sc.dtype.bool


def test_construct_0d_dtype():
    assert sc.Variable(2, dtype=np.int32).dtype == sc.dtype.int32
    assert sc.Variable(np.float64(2),
                       dtype=np.float32).dtype == sc.dtype.float32
    assert sc.Variable(1, dtype=np.bool).dtype == sc.dtype.bool


def test_rename_dims():
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    zy = sc.Variable(dims=['z', 'y'], values=values)
    xy.rename_dims({'x': 'z'})
    assert xy == zy


def test_create_1d_with_strings():
    v = sc.Variable(dims=['x'], values=["aaa", "ff", "bb"])
    assert np.all(v.values == np.array(["aaa", "ff", "bb"]))


def test_bool_variable_repr():
    a = sc.Variable(dims=['x'],
                    values=np.array([False, True, True, False, True]))
    assert [expected in repr(a) for expected in ["True", "False", "..."]]


def test_reciprocal():
    assert_export(sc.reciprocal, sc.Variable())


def test_reciprocal_out():
    var = sc.Variable()
    assert_export(sc.reciprocal, var, var)


def test_sin():
    assert_export(sc.sin, sc.Variable())


def test_sin_out():
    var = sc.Variable()
    assert_export(sc.sin, var, out=var)


def test_cos():
    assert_export(sc.cos, sc.Variable())


def test_cos_out():
    var = sc.Variable()
    assert_export(sc.cos, var, out=var)


def test_tan():
    assert_export(sc.tan, sc.Variable())


def test_tan_out():
    var = sc.Variable()
    assert_export(sc.tan, var, out=var)


def test_asin():
    assert_export(sc.asin, sc.Variable())


def test_asin_out():
    var = sc.Variable()
    assert_export(sc.asin, var, out=var)


def test_acos():
    assert_export(sc.acos, sc.Variable())


def test_acos_out():
    var = sc.Variable()
    assert_export(sc.acos, var, out=var)


def test_atan():
    assert_export(sc.atan, sc.Variable())


def test_atan_out():
    var = sc.Variable()
    assert_export(sc.atan, var, out=var)


def test_atan2():
    var = sc.Variable()
    assert_export(sc.atan2, y=var, x=var)
    assert_export(sc.atan2, y=var, x=var, out=var)


def test_variable_data_array_binary_ops():
    a = sc.DataArray(1.0 * sc.units.m)
    var = 1.0 * sc.units.m
    assert a / var == var / a


def test_num_to_nan():
    a = sc.Variable(dims=['x'], values=np.array([1, np.nan]))
    replace = sc.Variable(value=0.0)
    b = sc.nan_to_num(a, replace)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert b == expected


def test_nan_to_nan_with_pos_inf():
    a = sc.Variable(dims=['x'], values=np.array([1, np.inf]))
    replace = sc.Variable(value=0.0)
    b = sc.nan_to_num(a, posinf=replace)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert b == expected


def test_nan_to_nan_with_neg_inf():
    a = sc.Variable(dims=['x'], values=np.array([1, -np.inf]))
    replace = sc.Variable(value=0.0)
    b = sc.nan_to_num(a, neginf=replace)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert b == expected


def test_nan_to_nan_with_multiple_special_replacements():
    a = sc.Variable(dims=['x'], values=np.array([1, np.nan, np.inf, -np.inf]))
    replace_nan = sc.Variable(value=-1.0)
    replace_pos_inf = sc.Variable(value=-2.0)
    replace_neg_inf = sc.Variable(value=-3.0)
    b = sc.nan_to_num(a,
                      nan=replace_nan,
                      posinf=replace_pos_inf,
                      neginf=replace_neg_inf)

    expected = sc.Variable(
        dims=['x'],
        values=np.array([1] + [
            repl.value
            for repl in [replace_nan, replace_pos_inf, replace_neg_inf]
        ]))
    assert b == expected


def test_num_to_nan_out():
    a = sc.Variable(dims=['x'], values=np.array([1, np.nan]))
    out = sc.Variable(dims=['x'], values=np.zeros(2))
    replace = sc.Variable(value=0.0)
    sc.nan_to_num(a, nan=replace, out=out)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert out == expected


def test_num_to_nan_out_with_multiple_special_replacements():
    a = sc.Variable(dims=['x'], values=np.array([1, np.inf, -np.inf, np.nan]))
    out = sc.Variable(dims=['x'], values=np.zeros(4))
    replace = sc.Variable(value=0.0)
    # just replace nans
    sc.nan_to_num(a, nan=replace, out=out)
    expected = sc.Variable(dims=['x'],
                           values=np.array([1, np.inf, -np.inf,
                                            replace.value]))
    assert out == expected
    # replace neg inf
    sc.nan_to_num(out, neginf=replace, out=out)
    expected = sc.Variable(dims=['x'],
                           values=np.array(
                               [1, np.inf, replace.value, replace.value]))
    print(out, expected)
    assert out == expected
    # replace pos inf
    sc.nan_to_num(out, posinf=replace, out=out)
    expected = sc.Variable(dims=['x'],
                           values=np.array([1] + [replace.value] * 3))
    assert out == expected


def test_position():
    var = sc.Variable()
    assert_export(sc.geometry.position, x=var, y=var, z=var)


def test_x():
    var = sc.Variable()
    assert_export(sc.geometry.x, pos=var)


def test_y():
    var = sc.Variable()
    assert_export(sc.geometry.y, pos=var)


def test_z():
    var = sc.Variable()
    assert_export(sc.geometry.z, pos=var)


def test_less():
    var = sc.Variable()
    assert_export(sc.less, x=var, y=var)


def test_greater():
    var = sc.Variable()
    assert_export(sc.greater, x=var, y=var)


def test_greater_equal():
    var = sc.Variable()
    assert_export(sc.greater_equal, x=var, y=var)


def test_less_equal():
    var = sc.Variable()
    assert_export(sc.less_equal, x=var, y=var)


def test_equal():
    var = sc.Variable()
    assert_export(sc.equal, x=var, y=var)


def test_not_equal():
    var = sc.Variable()
    assert_export(sc.not_equal, x=var, y=var)


def test_radd_int():
    var = sc.Variable(dims=['x'], values=[1, 2, 3])
    assert (var + 1).dtype == var.dtype
    assert (1 + var).dtype == var.dtype


def test_rsub_int():
    var = sc.Variable(dims=['x'], values=[1, 2, 3])
    assert (var - 1).dtype == var.dtype
    assert (1 - var).dtype == var.dtype


def test_rmul_int():
    var = sc.Variable(dims=['x'], values=[1, 2, 3])
    assert (var * 1).dtype == var.dtype
    assert (1 * var).dtype == var.dtype


def test_rtruediv_int():
    var = sc.Variable(dims=['x'], values=[1, 2, 3])
    assert (var / 1).dtype == var.dtype
    assert (1 / var).dtype == var.dtype
