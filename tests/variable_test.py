# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Simon Heybrock
import os

import numpy as np
import numpy.typing as npt
import pytest

import scipp as sc


def make_variables() -> tuple[
    sc.Variable, sc.Variable, sc.Variable, sc.Variable, npt.NDArray[np.float64]
]:
    data = np.arange(1, 4, dtype=float)
    a = sc.Variable(dims=['x'], values=data)
    b = sc.Variable(dims=['x'], values=data)
    a_slice = a['x', :]
    b_slice = b['x', :]
    return a, b, a_slice, b_slice, data


def test_astype() -> None:
    var = sc.Variable(
        dims=['x'], values=np.array([1, 2, 3, 4], dtype=np.int64), unit='s'
    )
    assert var.dtype == sc.DType.int64
    assert var.unit == sc.units.s

    for target_dtype in (sc.DType.float64, float, 'float64'):
        var_as_float = var.astype(target_dtype)
        assert var_as_float.dtype == sc.DType.float64
        assert var_as_float.unit == sc.units.s


def test_astype_bad_conversion() -> None:
    var = sc.Variable(dims=['x'], values=np.array([1, 2, 3, 4], dtype=np.int64))
    assert var.dtype == sc.DType.int64

    for target_dtype in (sc.DType.string, str, 'str'):
        with pytest.raises(sc.DTypeError):
            var.astype(target_dtype)


def test_astype_datetime() -> None:
    var = sc.arange('x', np.datetime64(1, 's'), np.datetime64(5, 's'))
    assert var.dtype == sc.DType.datetime64
    assert var.unit == sc.units.s

    for target_dtype in (
        sc.DType.datetime64,
        np.datetime64,
        'datetime64',
        'datetime64[s]',
    ):
        same = var.astype(target_dtype)
        assert same.dtype == sc.DType.datetime64
        assert same.unit == sc.units.s


def test_astype_datetime_different_unit() -> None:
    var = sc.arange('x', np.datetime64(1, 's'), np.datetime64(5, 's'))
    assert var.unit == sc.units.s
    with pytest.raises(sc.UnitError):
        var.astype('datetime64[ms]')


def test_datetime_repr() -> None:
    # This test exists as windows time functions don't natively support times before
    # 1970-01-01, which has historically been overlooked and caused crashes.
    var = sc.datetimes(
        dims=["x"],
        values=[
            np.datetime64("1969-01-01T00:00:01", 's'),
            np.datetime64("1970-01-01T00:00:01", 's'),
        ],
    )

    if os.name == "nt":
        expected = "[(datetime before 1970, cannot format), 1970-01-01T00:00:01]"
    else:
        expected = "[1969-01-01T00:00:01, 1970-01-01T00:00:01]"

    assert repr(var).endswith(expected)


def test_operation_with_scalar_quantity() -> None:
    reference = sc.Variable(dims=['x'], values=np.arange(4.0) * 1.5)
    reference.unit = sc.units.kg

    var = sc.Variable(dims=['x'], values=np.arange(4.0))
    var *= sc.scalar(1.5, unit=sc.units.kg)
    assert sc.identical(reference, var)


def test_single_dim_access() -> None:
    var = sc.Variable(dims=['x'], values=[0.0])
    assert var.dim == 'x'
    assert isinstance(var.dim, str)
    assert var.sizes[var.dim] == 1


def test_0D_scalar_access() -> None:
    var = sc.Variable(dims=(), values=0.0)
    assert var.value == 0.0
    var.value = 1.2
    assert var.value == 1.2
    assert var.values.shape == ()
    assert var.values == 1.2


@pytest.mark.parametrize(
    'dtypes',
    [
        ('int32', np.int32),
        ('int64', np.int64),
        ('float32', np.float32),
        ('float64', np.float64),
        ('datetime64', np.datetime64),
        ('bool', np.bool_),
    ],
)
def test_0d_scalar_access_dtype(dtypes: tuple[str, type]) -> None:
    dtype, expected = dtypes
    assert isinstance(sc.scalar(7, unit='s', dtype=dtype).value, expected)


def test_0D_scalar_string() -> None:
    var = sc.scalar('a')
    assert var.value == 'a'
    var.value = 'b'
    assert sc.identical(var, sc.scalar('b'))


def test_1D_scalar_access_fail() -> None:
    var = sc.empty(dims=['x'], shape=(1,))
    with pytest.raises(sc.DimensionError):
        assert var.value == 0.0
    with pytest.raises(sc.DimensionError):
        var.value = 1.2


def test_1D_access_shape_mismatch_fail() -> None:
    var = sc.empty(dims=['x'], shape=(2,))
    with pytest.raises(sc.DimensionError):
        var.values = 1.2


def test_1D_access() -> None:
    var = sc.empty(dims=['x'], shape=(2,))
    assert len(var.values) == 2
    assert var.values.shape == (2,)
    var.values[1] = 1.2
    assert var.values[1] == 1.2


@pytest.mark.parametrize('dtype', ['int32', 'int64', 'float32', 'float64'])
def test_1d_access_dtype(dtype: str) -> None:
    assert sc.array(dims=['xx'], values=[-9], dtype=dtype).values.dtype == dtype


def test_1D_set_from_list() -> None:
    var = sc.empty(dims=['x'], shape=(2,))
    var.values = [1.0, 2.0]
    assert sc.identical(var, sc.Variable(dims=['x'], values=[1.0, 2.0]))


def test_1D_string() -> None:
    var = sc.Variable(dims=['x'], values=['a', 'b'])
    assert len(var.values) == 2
    assert var.values[0] == 'a'
    assert var.values[1] == 'b'
    var.values = ['c', 'd']
    assert sc.identical(var, sc.Variable(dims=['x'], values=['c', 'd']))


def test_1D_converting() -> None:
    var = sc.Variable(dims=['x'], values=[1, 2])
    var.values = [3.3, 4.6]
    # floats get truncated
    assert sc.identical(var, sc.Variable(dims=['x'], values=[3, 4]))


def test_1D_dataset() -> None:
    var = sc.empty(dims=['x'], shape=(2,), dtype=sc.DType.Dataset)
    d1 = sc.Dataset(data={'a': 1.5 * sc.units.m})
    d2 = sc.Dataset(data={'a': 2.5 * sc.units.m})
    var.values = [d1, d2]
    assert sc.identical(var.values[0], d1)
    assert sc.identical(var.values[1], d2)


def test_1D_access_bad_shape_fail() -> None:
    var = sc.empty(dims=['x'], shape=(2,))
    with pytest.raises(RuntimeError):
        var.values = np.arange(3)


def test_2D_access() -> None:
    var = sc.empty(dims=['x', 'y'], shape=(2, 3))
    assert var.values.shape == (2, 3)
    assert len(var.values) == 2
    assert len(var.values[0]) == 3
    var.values[1] = 1.2  # numpy assigns to all elements in "slice"
    var.values[1][2] = 2.2
    assert var.values[1][0] == 1.2
    assert var.values[1][1] == 1.2
    assert var.values[1][2] == 2.2


def test_2D_access_bad_shape_fail() -> None:
    var = sc.empty(dims=['x', 'y'], shape=(2, 3))
    with pytest.raises(RuntimeError):
        var.values = np.ones(shape=(3, 2))


def test_2D_access_variances() -> None:
    shape = (2, 3)
    var = sc.Variable(
        dims=['x', 'y'], values=np.full(shape, 29.0), variances=np.zeros(shape)
    )
    assert var.values.shape == (2, 3)
    assert var.variances.shape == (2, 3)
    var.values[1] = 1.2
    assert np.array_equal(var.variances, np.zeros(shape=shape))
    var.variances = np.ones(shape=shape)
    assert np.array_equal(var.variances, np.ones(shape=shape))


def test_getitem_element() -> None:
    var = sc.arange('a', 0, 8).fold('a', sizes={'x': 2, 'y': 4})
    for i in range(var.sizes['x']):
        assert sc.identical(var['x', i], sc.arange('y', 0 + i * 4, 4 + i * 4))
    for j in range(var.sizes['y']):
        assert sc.identical(var['y', j], sc.arange('x', 0 + j, 8 + j, 4))


def test_getitem_element_out_of_range() -> None:
    var = sc.arange('a', 0, 8).fold('a', sizes={'x': 2, 'y': 4})
    with pytest.raises(IndexError):
        _ = var['x', 4]
    with pytest.raises(IndexError):
        _ = var['y', 5]


def test_getitem_range() -> None:
    var = sc.arange('a', 0, 8).fold('a', sizes={'x': 2, 'y': 4})
    var_slice = var['x', 1:2]
    assert sc.identical(
        var_slice, sc.Variable(dims=['x', 'y'], values=np.arange(4, 8).reshape(1, 4))
    )


def test_setitem_broadcast() -> None:
    var = sc.Variable(dims=['x'], values=[1, 2, 3, 4], dtype=sc.DType.int64)
    var['x', 1:3] = sc.scalar(5, dtype=sc.DType.int64)
    assert sc.identical(
        var, sc.Variable(dims=['x'], values=[1, 5, 5, 4], dtype=sc.DType.int64)
    )


def test_slicing() -> None:
    var = sc.Variable(dims=['x'], values=np.arange(0, 3))
    for slice_, expected in (
        (slice(0, 2), [0, 1]),
        (slice(-3, -1), [0, 1]),
        (slice(2, 1), []),
    ):
        var_slice = var[('x', slice_)]
        assert len(var_slice.values) == len(expected)
        assert np.array_equal(var_slice.values, np.array(expected))


def test_sizes() -> None:
    a = sc.scalar(1)
    assert a.sizes == {}
    a = sc.empty(dims=['x'], shape=[2])
    assert a.sizes == {'x': 2}
    a = sc.empty(dims=['y', 'z'], shape=[3, 4])
    assert a.sizes == {'y': 3, 'z': 4}


def test_size() -> None:
    a = sc.scalar(1)
    assert a.size == 1
    a = sc.empty(dims=['x'], shape=[2])
    assert a.size == 2
    a = sc.empty(dims=['x', 'y'], shape=[3, 2])
    assert a.size == 6
    a = sc.empty(dims=['x', 'y'], shape=[0, 3])
    assert a.size == 0


def test_dims() -> None:
    a = sc.scalar(1)
    assert a.dims == ()
    a = sc.empty(dims=['x'], shape=[2])
    assert a.dims == ('x',)
    a = sc.empty(dims=['y', 'z'], shape=[3, 4])
    assert a.dims == ('y', 'z')


def test_concat() -> None:
    assert sc.identical(
        sc.concat([sc.scalar(1.0), sc.scalar(2.0)], 'a'),
        sc.array(dims=['a'], values=[1.0, 2.0]),
    )


def test_values_variances() -> None:
    assert sc.identical(sc.values(sc.scalar(1.2, variance=3.4)), sc.scalar(1.2))
    assert sc.identical(sc.variances(sc.scalar(1.2, variance=3.4)), sc.scalar(3.4))


def test_0d_variance_access_no_variance() -> None:
    v = sc.scalar(0.0)
    assert v.variance is None
    assert v.variances is None


def test_0d_variance_access() -> None:
    v = sc.scalar(0.0, variance=0.2)
    assert v.variance == 0.2
    assert v.variances == 0.2


def test_1D_scalar_variance_access_fail() -> None:
    v = sc.array(dims=['yy'], values=[0.0, -5.2], variances=[0.1, 2.2])
    with pytest.raises(sc.DimensionError):
        assert v.variance == 0.0
    with pytest.raises(sc.DimensionError):
        v.variance = 1.2


def test_1d_variance_access_no_variance() -> None:
    v = sc.array(dims=['yy'], values=[0.0])
    assert v.variances is None


def test_1d_variance_access() -> None:
    v = sc.array(dims=['yy'], values=[0.0, -5.2], variances=[0.1, 2.2])
    np.testing.assert_array_equal(v.variances, [0.1, 2.2])


@pytest.mark.parametrize('dtypes', [('float32', np.float32), ('float64', np.float64)])
def test_0d_scalar_variance_access_dtype(dtypes: tuple[str, type]) -> None:
    dtype, expected = dtypes
    assert isinstance(sc.scalar(4.1, variance=4.9, dtype=dtype).variance, expected)


@pytest.mark.parametrize('dtype', ['float32', 'float64'])
def test_1d_variance_access_dtype(dtype: str) -> None:
    assert (
        sc.array(
            dims=['xx'], values=[2.13], variances=[0.4], dtype=dtype
        ).variances.dtype
        == dtype
    )


def test_set_variance() -> None:
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sc.Variable(dims=['x', 'y'], values=values)
    expected = sc.Variable(dims=['x', 'y'], values=values, variances=variances)

    assert var.variances is None
    assert not sc.identical(var, expected)

    var.variances = variances

    assert var.variances is not None
    assert sc.identical(var, expected)


def test_copy_variance() -> None:
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sc.Variable(dims=['x', 'y'], values=values)
    expected = sc.Variable(dims=['x', 'y'], values=values, variances=variances)

    assert var.variances is None
    assert not sc.identical(var, expected)

    var.variances = expected.variances

    assert var.variances is not None
    assert sc.identical(var, expected)


def test_remove_variance() -> None:
    values = np.random.rand(2, 3)
    variances = np.random.rand(2, 3)
    var = sc.Variable(dims=['x', 'y'], values=values, variances=variances)
    expected = sc.Variable(dims=['x', 'y'], values=values)
    assert var.variances is not None
    var.variances = None
    assert var.variances is None
    assert sc.identical(var, expected)


def test_set_variance_convert_dtype() -> None:
    values = np.random.rand(2, 3)
    variances = np.arange(6).reshape(2, 3)
    assert variances.dtype == int
    var = sc.Variable(dims=['x', 'y'], values=values)
    expected = sc.Variable(
        dims=['x', 'y'], values=values, variances=variances, dtype=float
    )

    assert var.variances is None
    assert not sc.identical(var, expected)

    var.variances = variances

    assert var.variances is not None
    assert sc.identical(var, expected)


def test_rename_dims() -> None:
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    original = xy.copy()
    zy = sc.Variable(dims=['z', 'y'], values=values)
    assert sc.identical(xy.rename_dims({'x': 'z'}), zy)
    assert sc.identical(xy, original)


def test_rename_dims_kwargs() -> None:
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    original = xy.copy()
    zy = sc.Variable(dims=['z', 'y'], values=values)
    zw = sc.Variable(dims=['z', 'w'], values=values)
    assert sc.identical(xy.rename_dims(x='z'), zy)
    assert sc.identical(xy.rename_dims({'y': 'w'}, x='z'), zw)
    assert sc.identical(xy, original)


def test_rename_dims_dict_and_kwargs_must_be_distinct() -> None:
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    with pytest.raises(
        TypeError, match='The names passed in the dict and as keyword arguments'
    ):
        xy.rename_dims({'x': 'w'}, x='z')


def test_rename() -> None:
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    original = xy.copy()
    zy = sc.Variable(dims=['z', 'y'], values=values)
    assert sc.identical(xy.rename({'x': 'z'}), zy)
    assert sc.identical(xy, original)


def test_rename_kwargs() -> None:
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    original = xy.copy()
    zy = sc.Variable(dims=['z', 'y'], values=values)
    zw = sc.Variable(dims=['z', 'w'], values=values)
    assert sc.identical(xy.rename(x='z'), zy)
    assert sc.identical(xy.rename({'y': 'w'}, x='z'), zw)
    assert sc.identical(xy, original)


def test_rename_dict_and_kwargs_must_be_distinct() -> None:
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    with pytest.raises(
        TypeError, match='The names passed in the dict and as keyword arguments'
    ):
        xy.rename({'x': 'w'}, x='z')


def test_rename_self_and_dims_dict_are_position_only() -> None:
    values = np.arange(6).reshape(2, 3)
    xy = sc.Variable(dims=['x', 'y'], values=values)
    zy = sc.Variable(dims=['z', 'y'], values=values)
    # `var` is a position-only arg, so it *can* be used as a keyword
    assert sc.identical(xy.rename(x='var').rename(var='z'), zy)
    # `dims_dict` is a position-only arg, so it *can* be used as a keyword
    assert sc.identical(xy.rename(x='dims_dict').rename(dims_dict='z'), zy)


def test_bool_variable_repr() -> None:
    a = sc.Variable(dims=['x'], values=np.array([False, True, True, False, True]))
    assert [expected in repr(a) for expected in ["True", "False", "..."]]


def test_variable_data_array_binary_ops() -> None:
    a = sc.DataArray(1.0 * sc.units.m)
    var = 1.0 * sc.units.m
    assert sc.identical(a / var, var / a)


def test_isnan() -> None:
    assert sc.identical(
        sc.isnan(sc.Variable(dims=['x'], values=np.array([1, 1, np.nan]))),
        sc.Variable(dims=['x'], values=[False, False, True]),
    )


def test_isinf() -> None:
    assert sc.identical(
        sc.isinf(sc.Variable(dims=['x'], values=np.array([1, -np.inf, np.inf]))),
        sc.Variable(dims=['x'], values=[False, True, True]),
    )


def test_isfinite() -> None:
    assert sc.identical(
        sc.isfinite(
            sc.Variable(dims=['x'], values=np.array([1, -np.inf, np.inf, np.nan]))
        ),
        sc.Variable(dims=['x'], values=[True, False, False, False]),
    )


def test_isposinf() -> None:
    assert sc.identical(
        sc.isposinf(sc.Variable(dims=['x'], values=np.array([1, -np.inf, np.inf]))),
        sc.Variable(dims=['x'], values=[False, False, True]),
    )


def test_isneginf() -> None:
    assert sc.identical(
        sc.isneginf(sc.Variable(dims=['x'], values=np.array([1, -np.inf, np.inf]))),
        sc.Variable(dims=['x'], values=[False, True, False]),
    )


def test_nan_to_num() -> None:
    a = sc.Variable(dims=['x'], values=np.array([1, np.nan]))
    replace = sc.scalar(0.0)
    b = sc.nan_to_num(a, nan=replace)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert sc.identical(b, expected)


def test_nan_to_nan_with_pos_inf() -> None:
    a = sc.Variable(dims=['x'], values=np.array([1, np.inf]))
    replace = sc.scalar(0.0)
    b = sc.nan_to_num(a, posinf=replace)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert sc.identical(b, expected)


def test_nan_to_nan_with_neg_inf() -> None:
    a = sc.Variable(dims=['x'], values=np.array([1, -np.inf]))
    replace = sc.scalar(0.0)
    b = sc.nan_to_num(a, neginf=replace)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert sc.identical(b, expected)


def test_nan_to_nan_with_multiple_special_replacements() -> None:
    a = sc.Variable(dims=['x'], values=np.array([1, np.nan, np.inf, -np.inf]))
    replace_nan = sc.scalar(-1.0)
    replace_pos_inf = sc.scalar(-2.0)
    replace_neg_inf = sc.scalar(-3.0)
    b = sc.nan_to_num(
        a, nan=replace_nan, posinf=replace_pos_inf, neginf=replace_neg_inf
    )

    expected = sc.Variable(
        dims=['x'],
        values=np.array(
            [1]
            + [repl.value for repl in [replace_nan, replace_pos_inf, replace_neg_inf]]
        ),
    )
    assert sc.identical(b, expected)


def test_nan_to_num_out() -> None:
    a = sc.Variable(dims=['x'], values=np.array([1, np.nan]))
    out = sc.Variable(dims=['x'], values=np.zeros(2))
    replace = sc.scalar(0.0)
    sc.nan_to_num(a, nan=replace, out=out)
    expected = sc.Variable(dims=['x'], values=np.array([1, replace.value]))
    assert sc.identical(out, expected)


def test_nan_to_num_out_with_multiple_special_replacements() -> None:
    a = sc.Variable(dims=['x'], values=np.array([1, np.inf, -np.inf, np.nan]))
    out = sc.Variable(dims=['x'], values=np.zeros(4))
    replace = sc.scalar(0.0)
    # just replace nans
    sc.nan_to_num(a, nan=replace, out=out)
    expected = sc.Variable(
        dims=['x'], values=np.array([1, np.inf, -np.inf, replace.value])
    )
    assert sc.identical(out, expected)
    # replace neg inf
    sc.nan_to_num(out, neginf=replace, out=out)
    expected = sc.Variable(
        dims=['x'], values=np.array([1, np.inf, replace.value, replace.value])
    )
    assert sc.identical(out, expected)
    # replace pos inf
    sc.nan_to_num(out, posinf=replace, out=out)
    expected = sc.Variable(dims=['x'], values=np.array([1] + [replace.value] * 3))
    assert sc.identical(out, expected)


def test_sort() -> None:
    var = sc.array(dims=['s'], values=[3, 5, 1])
    assert sc.identical(
        sc.sort(var, key='s', order='ascending'), sc.array(dims=['s'], values=[1, 3, 5])
    )
    assert sc.identical(
        sc.sort(var, key='s', order='descending'),
        sc.array(dims=['s'], values=[5, 3, 1]),
    )


def test_issorted() -> None:
    assert sc.issorted(sc.arange('i', 4), dim='i', order='ascending')
    assert not sc.issorted(sc.arange('i', 4), dim='i', order='descending')


def test_allsorted() -> None:
    assert sc.allsorted(sc.arange('i', 4), dim='i', order='ascending')
    assert not sc.allsorted(sc.arange('i', 4), dim='i', order='descending')


@pytest.mark.parametrize('dtype', ['float64', 'float32', 'int64', 'int32'])
def test_islinspace_true(dtype: str) -> None:
    x = sc.Variable(dims=['x'], values=np.arange(5.0), unit=sc.units.m, dtype=dtype)
    assert sc.islinspace(x, 'x').value
    assert sc.islinspace(x).value


@pytest.mark.parametrize('dtype', ['float64', 'float32', 'int64', 'int32'])
def test_islinspace_false(dtype: str) -> None:
    x = sc.Variable(dims=['x'], values=(1, 1.5, 4), unit=sc.units.m, dtype=dtype)
    assert not sc.islinspace(x, 'x').value
    assert not sc.islinspace(x).value


def test_to_int64_to_float64() -> None:
    # If unit conversion is done first followed by dtype conversion, this will be lossy.
    data = sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="m")

    assert sc.identical(
        data.to(unit="km", dtype="float64"),
        sc.array(dims=["x"], values=[0.001, 0.002, 0.003], dtype="float64", unit="km"),
    )


def test_to_int64_to_float32() -> None:
    # If unit conversion is done first followed by dtype conversion, this will be lossy.
    data = sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="m")

    assert sc.identical(
        data.to(unit="km", dtype="float32"),
        sc.array(dims=["x"], values=[0.001, 0.002, 0.003], dtype="float32", unit="km"),
    )


def test_to_float64_to_int64() -> None:
    # Will be lossy if dtype conversion done before unit conversion
    data = sc.array(dims=["x"], values=[0.001, 0.002, 0.003], dtype="float64", unit="m")

    assert sc.identical(
        data.to(unit="mm", dtype="int64"),
        sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="mm"),
    )


def test_to_float32_to_int64() -> None:
    # Will be lossy if dtype conversion done before unit conversion
    data = sc.array(dims=["x"], values=[0.001, 0.002, 0.003], dtype="float32", unit="m")

    assert sc.identical(
        data.to(unit="mm", dtype="int64"),
        sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="mm"),
    )


def test_to_int64_to_int32() -> None:
    # Will be lossy if dtype conversion done first
    data = sc.array(dims=["x"], values=[1000 * 2**30], dtype="int64", unit="m")

    assert sc.identical(
        data.to(unit="km", dtype="int32"),
        sc.array(dims=["x"], values=[2**30], dtype="int32", unit="km"),
    )


def test_to_int32_to_int64() -> None:
    # Will be lossy if unit conversion done first
    data = sc.array(dims=["x"], values=[2**30], dtype="int32", unit="m")

    assert sc.identical(
        data.to(unit="mm", dtype="int64"),
        sc.array(dims=["x"], values=[1000 * 2**30], dtype="int64", unit="mm"),
    )


def test_to_without_unit() -> None:
    data = sc.array(dims=["x"], values=[1, 2, 3], dtype="int32", unit="m")

    assert sc.identical(
        data.to(dtype="int64"),
        sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="m"),
    )


def test_to_without_dtype() -> None:
    data = sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="m")

    assert sc.identical(
        data.to(unit="mm"),
        sc.array(dims=["x"], values=[1000, 2000, 3000], dtype="int64", unit="mm"),
    )


def test_to_without_any_arguments() -> None:
    data = sc.array(dims=["x"], values=[1, 2, 3], dtype="int64", unit="m")

    with pytest.raises(ValueError, match='Must provide dtype or unit or both'):
        data.to()


def test_setting_float_unit_property_to_default_gives_dimensionless() -> None:
    var = sc.scalar(1.2, unit='m')
    var.unit = sc.units.default_unit
    assert var.unit == sc.units.one


def test_setting_string_unit_property_to_default_gives_none() -> None:
    var = sc.scalar('abc', unit='m')
    var.unit = sc.units.default_unit
    assert var.unit is None


def test_aligned_default() -> None:
    var = sc.scalar(1)
    assert var.aligned


def test_cannot_set_aligned_flag() -> None:
    # Cannot set alignment like this because it would be easy to make mistakes.
    # I.e. `da.coords['x'].aligned = False` has no effect because Coords.__getitem__
    # returns a copy of the variable.
    var = sc.scalar(1)
    with pytest.raises(AttributeError):
        var.aligned = False  # type: ignore[misc]
