# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import os

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


def test_astype():
    var = sc.Variable(dims=['x'],
                      values=np.array([1, 2, 3, 4], dtype=np.int64),
                      unit='s')
    assert var.dtype == sc.DType.int64
    assert var.unit == sc.units.s

    for target_dtype in (sc.DType.float64, float, 'float64'):
        var_as_float = var.astype(target_dtype)
        assert var_as_float.dtype == sc.DType.float64
        assert var_as_float.unit == sc.units.s


def test_astype_bad_conversion():
    var = sc.Variable(dims=['x'], values=np.array([1, 2, 3, 4], dtype=np.int64))
    assert var.dtype == sc.DType.int64

    for target_dtype in (sc.DType.string, str, 'str'):
        with pytest.raises(sc.DTypeError):
            var.astype(target_dtype)


def test_astype_datetime():
    var = sc.arange('x', np.datetime64(1, 's'), np.datetime64(5, 's'))
    assert var.dtype == sc.DType.datetime64
    assert var.unit == sc.units.s

    for target_dtype in (sc.DType.datetime64, np.datetime64, 'datetime64',
                         'datetime64[s]'):
        same = var.astype(target_dtype)
        assert same.dtype == sc.DType.datetime64
        assert same.unit == sc.units.s


def test_astype_datetime_different_unit():
    var = sc.arange('x', np.datetime64(1, 's'), np.datetime64(5, 's'))
    assert var.unit == sc.units.s
    with pytest.raises(sc.UnitError):
        var.astype('datetime64[ms]')


def test_datetime_repr():
    # This test exists as windows time functions don't natively support times before
    # 1970-01-01, which has historically been overlooked and caused crashes.
    var = sc.datetimes(dims=["x"],
                       values=[
                           np.datetime64("1969-01-01T00:00:01", 's'),
                           np.datetime64("1970-01-01T00:00:01", 's')
                       ])

    if os.name == "nt":
        expected = "[(datetime before 1970, cannot format), 1970-01-01T00:00:01]"
    else:
        expected = "[1969-01-01T00:00:01, 1970-01-01T00:00:01]"

    assert repr(var).endswith(expected)


def test_operation_with_scalar_quantity():
    reference = sc.Variable(dims=['x'], values=np.arange(4.0) * 1.5)
    reference.unit = sc.units.kg

    var = sc.Variable(dims=['x'], values=np.arange(4.0))
    var *= sc.scalar(1.5, unit=sc.units.kg)
    assert sc.identical(reference, var)


def test_single_dim_access():
    var = sc.Variable(dims=['x'], values=[0.0])
    assert var.dim == 'x'
    assert isinstance(var.dim, str)
    assert var.sizes[var.dim] == 1


def test_0D_scalar_access():
    var = sc.Variable(dims=(), values=0.0)
    assert var.value == 0.0
    var.value = 1.2
    assert var.value == 1.2
    assert var.values.shape == ()
    assert var.values == 1.2


def test_0D_scalar_string():
    var = sc.scalar('a')
    assert var.value == 'a'
    var.value = 'b'
    assert sc.identical(var, sc.scalar('b'))


def test_1D_scalar_access_fail():
    var = sc.empty(dims=['x'], shape=(1, ))
    with pytest.raises(RuntimeError):
        assert var.value == 0.0
    with pytest.raises(RuntimeError):
        var.value = 1.2


def test_1D_access_shape_mismatch_fail():
    var = sc.empty(dims=['x'], shape=(2, ))
    with pytest.raises(RuntimeError):
        var.values = 1.2


def test_1D_access():
    var = sc.empty(dims=['x'], shape=(2, ))
    assert len(var.values) == 2
    assert var.values.shape == (2, )
    var.values[1] = 1.2
    assert var.values[1] == 1.2


def test_1D_set_from_list():
    var = sc.empty(dims=['x'], shape=(2, ))
    var.values = [1.0, 2.0]
    assert sc.identical(var, sc.Variable(dims=['x'], values=[1.0, 2.0]))


def test_1D_string():
    var = sc.Variable(dims=['x'], values=['a', 'b'])
    assert len(var.values) == 2
    assert var.values[0] == 'a'
    assert var.values[1] == 'b'
    var.values = ['c', 'd']
    assert sc.identical(var, sc.Variable(dims=['x'], values=['c', 'd']))


def test_1D_converting():
    var = sc.Variable(dims=['x'], values=[1, 2])
    var.values = [3.3, 4.6]
    # floats get truncated
    assert sc.identical(var, sc.Variable(dims=['x'], values=[3, 4]))


def test_1D_dataset():
    var = sc.empty(dims=['x'], shape=(2, ), dtype=sc.DType.Dataset)
    d1 = sc.Dataset(data={'a': 1.5 * sc.units.m})
    d2 = sc.Dataset(data={'a': 2.5 * sc.units.m})
    var.values = [d1, d2]
    assert sc.identical(var.values[0], d1)
    assert sc.identical(var.values[1], d2)


def test_1D_access_bad_shape_fail():
    var = sc.empty(dims=['x'], shape=(2, ))
    with pytest.raises(RuntimeError):
        var.values = np.arange(3)


def test_2D_access():
    var = sc.empty(dims=['x', 'y'], shape=(2, 3))
    assert var.values.shape == (2, 3)
    assert len(var.values) == 2
    assert len(var.values[0]) == 3
    var.values[1] = 1.2  # numpy assigns to all elements in "slice"
    var.values[1][2] = 2.2
    assert var.values[1][0] == 1.2
    assert var.values[1][1] == 1.2
    assert var.values[1][2] == 2.2


def test_2D_access_bad_shape_fail():
    var = sc.empty(dims=['x', 'y'], shape=(2, 3))
    with pytest.raises(RuntimeError):
        var.values = np.ones(shape=(3, 2))


def test_2D_access_variances():
    shape = (2, 3)
    var = sc.Variable(dims=['x', 'y'],
                      values=np.full(shape, 29.0),
                      variances=np.zeros(shape))
    assert var.values.shape == (2, 3)
    assert var.variances.shape == (2, 3)
    var.values[1] = 1.2
    assert np.array_equal(var.variances, np.zeros(shape=shape))
    var.variances = np.ones(shape=shape)
    assert np.array_equal(var.variances, np.ones(shape=shape))


def test_getitem_element():
    var = sc.arange('a', 0, 8).fold('a', {'x': 2, 'y': 4})
    for i in range(var.sizes['x']):
        assert sc.identical(var['x', i], sc.arange('y', 0 + i * 4, 4 + i * 4))
    for j in range(var.sizes['y']):
        assert sc.identical(var['y', j], sc.arange('x', 0 + j, 8 + j, 4))


def test_getitem_element_out_of_range():
    var = sc.arange('a', 0, 8).fold('a', {'x': 2, 'y': 4})
    with pytest.raises(IndexError):
        _ = var['x', 4]
    with pytest.raises(IndexError):
        _ = var['y', 5]


def test_getitem_range():
    var = sc.arange('a', 0, 8).fold('a', {'x': 2, 'y': 4})
    var_slice = var['x', 1:2]
    assert sc.identical(
        var_slice, sc.Variable(dims=['x', 'y'], values=np.arange(4, 8).reshape(1, 4)))


def test_setitem_broadcast():
    var = sc.Variable(dims=['x'], values=[1, 2, 3, 4], dtype=sc.DType.int64)
    var['x', 1:3] = sc.scalar(5, dtype=sc.DType.int64)
    assert sc.identical(
        var, sc.Variable(dims=['x'], values=[1, 5, 5, 4], dtype=sc.DType.int64))


def test_slicing():
    var = sc.Variable(dims=['x'], values=np.arange(0, 3))
    for slice_, expected in ((slice(0, 2), [0, 1]), (slice(-3,
                                                           -1), [0,
                                                                 1]), (slice(2,
                                                                             1), [])):
        var_slice = var[('x', slice_)]
        assert len(var_slice.values) == len(expected)
        assert np.array_equal(var_slice.values, np.array(expected))


def test_sizes():
    a = sc.scalar(1)
    assert a.sizes == {}
    a = sc.empty(dims=['x'], shape=[2])
    assert a.sizes == {'x': 2}
    a = sc.empty(dims=['y', 'z'], shape=[3, 4])
    assert a.sizes == {'y': 3, 'z': 4}


def test_iadd():
    expected = sc.scalar(2.2)
    a = sc.scalar(1.2)
    b = a
    a += 1.0
    assert sc.identical(a, expected)
    assert sc.identical(b, expected)
    # This extra check is important: It can happen that an implementation of,
    # e.g., __iadd__ does an in-place modification, updating `b`, but then the
    # return value is assigned to `a`, which could break the connection unless
    # the correct Python object is returned.
    a += 1.0
    assert sc.identical(a, b)


def test_isub():
    expected = sc.scalar(2.2 - 1.0)
    a = sc.scalar(2.2)
    b = a
    a -= 1.0
    assert sc.identical(a, expected)
    assert sc.identical(b, expected)
    a -= 1.0
    assert sc.identical(a, b)


def test_imul():
    expected = sc.scalar(2.4)
    a = sc.scalar(1.2)
    b = a
    a *= 2.0
    assert sc.identical(a, expected)
    assert sc.identical(b, expected)
    a *= 2.0
    assert sc.identical(a, b)


def test_idiv():
    expected = sc.scalar(1.2)
    a = sc.scalar(2.4)
    b = a
    a /= 2.0
    assert sc.identical(a, expected)
    assert sc.identical(b, expected)
    a /= 2.0
    assert sc.identical(a, b)


def test_iand():
    expected = sc.scalar(False)
    a = sc.scalar(True)
    b = a
    a &= sc.scalar(False)
    assert sc.identical(a, expected)
    assert sc.identical(b, expected)
    a |= sc.scalar(True)
    assert sc.identical(a, b)


def test_ior():
    expected = sc.scalar(True)
    a = sc.scalar(False)
    b = a
    a |= sc.scalar(True)
    assert sc.identical(a, expected)
    assert sc.identical(b, expected)
    a &= sc.scalar(True)
    assert sc.identical(a, b)


def test_ixor():
    expected = sc.scalar(True)
    a = sc.scalar(False)
    b = a
    a ^= sc.scalar(True)
    assert sc.identical(a, expected)
    assert sc.identical(b, expected)
    a ^= sc.scalar(True)
    assert sc.identical(a, b)


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


def test_binary_pow():
    a, b, a_slice, b_slice, data = make_variables()
    c = a**b
    assert np.array_equal(c.values, data**data)
    c **= b
    assert np.array_equal(c.values, (data**data)**data)
    c = a**3
    assert np.array_equal(c.values, data**3)
    c **= 3
    assert np.array_equal(c.values, (data**3)**3)
    c = a**3.0
    assert np.array_equal(c.values, data**3.0)
    c **= 3.0
    assert np.array_equal(c.values, (data**3.0)**3.0)
    c = a**b_slice
    assert np.array_equal(c.values, data**data)
    c **= b_slice
    assert np.array_equal(c.values, (data**data)**data)
    c = 2**b
    assert np.array_equal(c.values, 2**data)
    c = 2.0**b
    assert np.array_equal(c.values, 2.0**data)


def test_in_place_binary_or():
    a = sc.scalar(False)
    b = sc.scalar(True)
    a |= b
    assert sc.identical(a, sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a |= b
    assert sc.identical(
        a, sc.Variable(dims=['x'], values=np.array([False, True, True, True])))


def test_binary_or():
    a = sc.scalar(False)
    b = sc.scalar(True)
    assert sc.identical((a | b), sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert sc.identical((a | b),
                        sc.Variable(dims=['x'],
                                    values=np.array([False, True, True, True])))


def test_in_place_binary_and():
    a = sc.scalar(False)
    b = sc.scalar(True)
    a &= b
    assert sc.identical(a, sc.scalar(False))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a &= b
    assert sc.identical(
        a, sc.Variable(dims=['x'], values=np.array([False, False, False, True])))


def test_binary_and():
    a = sc.scalar(False)
    b = sc.scalar(True)
    assert sc.identical((a & b), sc.scalar(False))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert sc.identical((a & b),
                        sc.Variable(dims=['x'],
                                    values=np.array([False, False, False, True])))


def test_in_place_binary_xor():
    a = sc.scalar(False)
    b = sc.scalar(True)
    a ^= b
    assert sc.identical(a, sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    a ^= b
    assert sc.identical(
        a, sc.Variable(dims=['x'], values=np.array([False, True, True, False])))


def test_binary_xor():
    a = sc.scalar(False)
    b = sc.scalar(True)
    assert sc.identical((a ^ b), sc.scalar(True))

    a = sc.Variable(dims=['x'], values=np.array([False, True, False, True]))
    b = sc.Variable(dims=['x'], values=np.array([False, False, True, True]))
    assert sc.identical((a ^ b),
                        sc.Variable(dims=['x'],
                                    values=np.array([False, True, True, False])))


def test_in_place_binary_with_scalar():
    v = sc.Variable(dims=['x'], values=[10.0])
    copy = v.copy()

    v += 2
    v *= 2
    v -= 4
    v /= 2
    assert sc.identical(v, copy)


def test_binary_equal():
    a, b, a_slice, b_slice, data = make_variables()
    assert sc.identical(a, b)
    assert sc.identical(a, a_slice)
    assert sc.identical(a_slice, b_slice)
    assert sc.identical(b, a)
    assert sc.identical(b_slice, a)
    assert sc.identical(b_slice, a_slice)


def test_binary_not_equal():
    a, b, a_slice, b_slice, data = make_variables()
    c = a + b
    assert not sc.identical(a, c)
    assert not sc.identical(a_slice, c)
    assert not sc.identical(c, a)
    assert not sc.identical(c, a_slice)


def test_concat():
    assert_export(sc.concat,
                  [sc.Variable(dims=(), values=0.0),
                   sc.Variable(dims=(), values=0.0)], 'x')


def test_mean():
    assert_export(sc.mean, sc.Variable(dims=(), values=0.0), 'x')


def test_mean_in_place():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.mean, sc.Variable(dims=(), values=0.0), 'x', var)


def test_values_variances():
    assert_export(sc.values, sc.Variable(dims=(), values=0.0))
    assert_export(sc.variances, sc.Variable(dims=(), values=0.0))


def test_sum():
    var = sc.Variable(dims=['x', 'y'],
                      values=np.array([[0.1, 0.3], [0.2, 0.6]]),
                      unit=sc.units.m)
    expected = sc.Variable(dims=['x'], values=np.array([0.4, 0.8]), unit=sc.units.m)
    assert sc.identical(sc.sum(var, 'y'), expected)


def test_sum_in_place():
    var = sc.Variable(dims=['x', 'y'],
                      values=np.array([[0.1, 0.3], [0.2, 0.6]]),
                      unit=sc.units.m)
    out_var = sc.Variable(dims=['x'], values=np.array([0.0, 0.0]), unit=sc.units.m)
    expected = sc.Variable(dims=['x'], values=np.array([0.4, 0.8]), unit=sc.units.m)
    out_view = sc.sum(var, 'y', out=out_var)
    assert sc.identical(out_var, expected)
    assert sc.identical(out_view, expected)


def test_variance_acess():
    v = sc.Variable(dims=(), values=0.0)
    assert v.variance is None
    assert v.variances is None


def test_set_variance():
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sc.Variable(dims=['x', 'y'], values=values)
    expected = sc.Variable(dims=['x', 'y'], values=values, variances=variances)

    assert var.variances is None
    assert not sc.identical(var, expected)

    var.variances = variances

    assert var.variances is not None
    assert sc.identical(var, expected)


def test_copy_variance():
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sc.Variable(dims=['x', 'y'], values=values)
    expected = sc.Variable(dims=['x', 'y'], values=values, variances=variances)

    assert var.variances is None
    assert not sc.identical(var, expected)

    var.variances = expected.variances

    assert var.variances is not None
    assert sc.identical(var, expected)


def test_remove_variance():
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sc.Variable(dims=['x', 'y'], values=values, variances=variances)
    expected = sc.Variable(dims=['x', 'y'], values=values)
    assert var.variances is not None
    var.variances = None
    assert var.variances is None
    assert sc.identical(var, expected)


def test_set_variance_convert_dtype():
    values = np.random.rand(2, 3)
    variances = np.arange(6).reshape(2, 3)
    assert variances.dtype == int
    var = sc.Variable(dims=['x', 'y'], values=values)
    expected = sc.Variable(dims=['x', 'y'],
                           values=values,
                           variances=variances,
                           dtype=float)

    assert var.variances is None
    assert not sc.identical(var, expected)

    var.variances = variances

    assert var.variances is not None
    assert sc.identical(var, expected)


def test_sum_mean():
    var = sc.Variable(dims=['x'], values=np.arange(5, dtype=np.int64))
    assert sc.identical(sc.sum(var, 'x'), sc.scalar(10))
    var = sc.Variable(dims=['x'], values=np.arange(6, dtype=np.int64))
    assert sc.identical(sc.mean(var, 'x'), sc.scalar(2.5))


def test_rename_dims():
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    original = xy.copy()
    zy = sc.Variable(dims=['z', 'y'], values=values)
    assert sc.identical(xy.rename_dims({'x': 'z'}), zy)
    assert sc.identical(xy, original)


def test_rename():
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    original = xy.copy()
    zy = sc.Variable(dims=['z', 'y'], values=values)
    assert sc.identical(xy.rename({'x': 'z'}), zy)
    assert sc.identical(xy, original)


def test_rename_kwargs():
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    zy = sc.Variable(dims=['z', 'y'], values=values)
    assert sc.identical(xy.rename(x='z'), zy)


def test_bool_variable_repr():
    a = sc.Variable(dims=['x'], values=np.array([False, True, True, False, True]))
    assert [expected in repr(a) for expected in ["True", "False", "..."]]


def test_sin():
    assert_export(sc.sin, sc.Variable(dims=(), values=0.0))


def test_sin_out():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.sin, var, out=var)


def test_cos():
    assert_export(sc.cos, sc.Variable(dims=(), values=0.0))


def test_cos_out():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.cos, var, out=var)


def test_tan():
    assert_export(sc.tan, sc.Variable(dims=(), values=0.0))


def test_tan_out():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.tan, var, out=var)


def test_asin():
    assert_export(sc.asin, sc.Variable(dims=(), values=0.0))


def test_asin_out():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.asin, var, out=var)


def test_acos():
    assert_export(sc.acos, sc.Variable(dims=(), values=0.0))


def test_acos_out():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.acos, var, out=var)


def test_atan():
    assert_export(sc.atan, sc.Variable(dims=(), values=0.0))


def test_atan_out():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.atan, var, out=var)


def test_atan2():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.atan2, y=var, x=var)
    assert_export(sc.atan2, y=var, x=var, out=var)


def test_variable_data_array_binary_ops():
    a = sc.DataArray(1.0 * sc.units.m)
    var = 1.0 * sc.units.m
    assert sc.identical(a / var, var / a)


def test_isnan():
    assert sc.identical(
        sc.isnan(sc.Variable(dims=['x'], values=np.array([1, 1, np.nan]))),
        sc.Variable(dims=['x'], values=[False, False, True]))


def test_isinf():
    assert sc.identical(
        sc.isinf(sc.Variable(dims=['x'], values=np.array([1, -np.inf, np.inf]))),
        sc.Variable(dims=['x'], values=[False, True, True]))


def test_isfinite():
    assert sc.identical(
        sc.isfinite(
            sc.Variable(dims=['x'], values=np.array([1, -np.inf, np.inf, np.nan]))),
        sc.Variable(dims=['x'], values=[True, False, False, False]))


def test_isposinf():
    assert sc.identical(
        sc.isposinf(sc.Variable(dims=['x'], values=np.array([1, -np.inf, np.inf]))),
        sc.Variable(dims=['x'], values=[False, False, True]))


def test_isneginf():
    assert sc.identical(
        sc.isneginf(sc.Variable(dims=['x'], values=np.array([1, -np.inf, np.inf]))),
        sc.Variable(dims=['x'], values=[False, True, False]))


def test_nan_to_num():
    a = sc.Variable(dims=['x'], values=np.array([1, np.nan]))
    replace = sc.scalar(0.0)
    b = sc.nan_to_num(a, nan=replace)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert sc.identical(b, expected)


def test_nan_to_nan_with_pos_inf():
    a = sc.Variable(dims=['x'], values=np.array([1, np.inf]))
    replace = sc.scalar(0.0)
    b = sc.nan_to_num(a, posinf=replace)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert sc.identical(b, expected)


def test_nan_to_nan_with_neg_inf():
    a = sc.Variable(dims=['x'], values=np.array([1, -np.inf]))
    replace = sc.scalar(0.0)
    b = sc.nan_to_num(a, neginf=replace)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert sc.identical(b, expected)


def test_nan_to_nan_with_multiple_special_replacements():
    a = sc.Variable(dims=['x'], values=np.array([1, np.nan, np.inf, -np.inf]))
    replace_nan = sc.scalar(-1.0)
    replace_pos_inf = sc.scalar(-2.0)
    replace_neg_inf = sc.scalar(-3.0)
    b = sc.nan_to_num(a,
                      nan=replace_nan,
                      posinf=replace_pos_inf,
                      neginf=replace_neg_inf)

    expected = sc.Variable(
        dims=['x'],
        values=np.array(
            [1] +
            [repl.value for repl in [replace_nan, replace_pos_inf, replace_neg_inf]]))
    assert sc.identical(b, expected)


def test_nan_to_num_out():
    a = sc.Variable(dims=['x'], values=np.array([1, np.nan]))
    out = sc.Variable(dims=['x'], values=np.zeros(2))
    replace = sc.scalar(0.0)
    sc.nan_to_num(a, nan=replace, out=out)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert sc.identical(out, expected)


def test_nan_to_num_out_with_multiple_special_replacements():
    a = sc.Variable(dims=['x'], values=np.array([1, np.inf, -np.inf, np.nan]))
    out = sc.Variable(dims=['x'], values=np.zeros(4))
    replace = sc.scalar(0.0)
    # just replace nans
    sc.nan_to_num(a, nan=replace, out=out)
    expected = sc.Variable(dims=['x'],
                           values=np.array([1, np.inf, -np.inf, replace.value]))
    assert sc.identical(out, expected)
    # replace neg inf
    sc.nan_to_num(out, neginf=replace, out=out)
    expected = sc.Variable(dims=['x'],
                           values=np.array([1, np.inf, replace.value, replace.value]))
    assert sc.identical(out, expected)
    # replace pos inf
    sc.nan_to_num(out, posinf=replace, out=out)
    expected = sc.Variable(dims=['x'], values=np.array([1] + [replace.value] * 3))
    assert sc.identical(out, expected)


def test_position():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.geometry.position, x=var, y=var, z=var)


def test_comparison():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.less, x=var, y=var)
    assert_export(sc.greater, x=var, y=var)
    assert_export(sc.greater_equal, x=var, y=var)
    assert_export(sc.less_equal, x=var, y=var)
    assert_export(sc.equal, x=var, y=var)
    assert_export(sc.not_equal, x=var, y=var)


def test_radd_int():
    var = sc.Variable(dims=['x'], values=[1, 2, 3])
    assert (var + 1).dtype == sc.DType.int64
    assert (1 + var).dtype == sc.DType.int64


def test_rsub_int():
    var = sc.Variable(dims=['x'], values=[1, 2, 3])
    assert (var - 1).dtype == sc.DType.int64
    assert (1 - var).dtype == sc.DType.int64


def test_rmul_int():
    var = sc.Variable(dims=['x'], values=[1, 2, 3])
    assert (var * 1).dtype == sc.DType.int64
    assert (1 * var).dtype == sc.DType.int64


def test_rtruediv_int():
    var = sc.Variable(dims=['x'], values=[1, 2, 3])
    assert (var / 1).dtype == sc.DType.float64
    assert (1 / var).dtype == sc.DType.float64


def test_sort():
    var = sc.Variable(dims=(), values=0.0)
    assert_export(sc.sort, x=var, dim='x', order='ascending')
    assert_export(sc.issorted, x=var, dim='x', order='ascending')
    assert_export(sc.allsorted, x=var, dim='x', order='ascending')


def test_islinspace_true():
    x = sc.Variable(dims=['x'], values=np.arange(5.), unit=sc.units.m)
    assert sc.islinspace(x, 'x').value
    assert sc.islinspace(x).value


def test_islinspace_false():
    x = sc.Variable(dims=['x'], values=(1, 1.5, 4), unit=sc.units.m)
    assert not sc.islinspace(x, 'x').value
    assert not sc.islinspace(x).value


def test_to_int64_to_float64():
    # If unit conversion is done first followed by dtype conversion, this will be lossy.
    data = sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="m")

    assert sc.identical(
        data.to(unit="km", dtype="float64"),
        sc.array(dims=["x"], values=[0.001, 0.002, 0.003], dtype="float64", unit="km"))


def test_to_int64_to_float32():
    # If unit conversion is done first followed by dtype conversion, this will be lossy.
    data = sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="m")

    assert sc.identical(
        data.to(unit="km", dtype="float32"),
        sc.array(dims=["x"], values=[0.001, 0.002, 0.003], dtype="float32", unit="km"))


def test_to_float64_to_int64():
    # Will be lossy if dtype conversion done before unit conversion
    data = sc.array(dims=["x"], values=[0.001, 0.002, 0.003], dtype="float64", unit="m")

    assert sc.identical(
        data.to(unit="mm", dtype="int64"),
        sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="mm"))


def test_to_float32_to_int64():
    # Will be lossy if dtype conversion done before unit conversion
    data = sc.array(dims=["x"], values=[0.001, 0.002, 0.003], dtype="float32", unit="m")

    assert sc.identical(
        data.to(unit="mm", dtype="int64"),
        sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="mm"))


def test_to_int64_to_int32():
    # Will be lossy if dtype conversion done first
    data = sc.array(dims=["x"], values=[1000 * 2**30], dtype="int64", unit="m")

    assert sc.identical(data.to(unit="km", dtype="int32"),
                        sc.array(dims=["x"], values=[2**30], dtype="int32", unit="km"))


def test_to_int32_to_int64():
    # Will be lossy if unit conversion done first
    data = sc.array(dims=["x"], values=[2**30], dtype="int32", unit="m")

    assert sc.identical(
        data.to(unit="mm", dtype="int64"),
        sc.array(dims=["x"], values=[1000 * 2**30], dtype="int64", unit="mm"))


def test_to_without_unit():
    data = sc.array(dims=["x"], values=[1, 2, 3], dtype="int32", unit="m")

    assert sc.identical(data.to(dtype="int64"),
                        sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="m"))


def test_to_without_dtype():
    data = sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="m")

    assert sc.identical(
        data.to(unit="mm"),
        sc.array(dims=["x"], values=[1000, 2000, 3000], dtype="int64", unit="mm"))


def test_to_without_any_arguments():
    data = sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="m")

    with pytest.raises(ValueError):
        data.to()


def test_setting_float_unit_property_to_default_gives_dimensionless():
    var = sc.scalar(1.2, unit='m')
    var.unit = sc.units.default_unit
    assert var.unit == sc.units.one


def test_setting_string_unit_property_to_default_gives_none():
    var = sc.scalar('abc', unit='m')
    var.unit = sc.units.default_unit
    assert var.unit is None
