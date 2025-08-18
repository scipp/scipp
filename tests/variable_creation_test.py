# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from collections.abc import Callable, Sequence
from typing import Any

import numpy as np
import numpy.typing as npt
import pytest

import scipp as sc

# https://numpy.org/devdocs/release/2.0.0-notes.html#remove-datetime64-deprecation-warning-when-constructing-with-timezone
# TODO: This should be removed once we drop support for numpy<2
raise_np_datetime = (
    ValueError if np.lib.NumpyVersion(np.__version__) < '2.0.0b1' else UserWarning
)


def _compare_properties(a: sc.Variable, b: sc.Variable) -> None:
    assert a.dims == b.dims
    assert a.shape == b.shape
    assert a.unit == b.unit
    assert a.dtype == b.dtype
    assert (a.variances is None) == (b.variances is None)


def make_dummy(
    dims: Sequence[str],
    shape: Sequence[int],
    with_variances: bool = False,
    **kwargs: Any,
) -> sc.Variable:
    # Not using empty to avoid a copy from uninitialized memory in `expected`.
    if with_variances:
        return sc.Variable(
            dims=dims,
            values=np.full(shape, 63.0),
            variances=np.full(shape, 12.0),
            **kwargs,
        )
    return sc.Variable(dims=dims, values=np.full(shape, 81.0), **kwargs)


def test_scalar_with_dtype() -> None:
    value = 1.0
    variance = 5.0
    unit = sc.units.m
    dtype = sc.DType.float64
    var = sc.scalar(value=value, variance=variance, unit=unit, dtype=dtype)
    expected = sc.Variable(
        dims=(), values=value, variances=variance, unit=unit, dtype=dtype
    )
    assert sc.identical(var, expected)


def test_scalar_float_default_unit_is_dimensionless() -> None:
    assert sc.scalar(value=1.2).unit == sc.units.one


def test_scalar_string_default_unit_is_None() -> None:
    assert sc.scalar(value='abc').unit is None


def test_scalar_without_dtype() -> None:
    value = 'temp'
    var = sc.scalar(value)
    expected = sc.Variable(dims=(), values=value)
    assert sc.identical(var, expected)


def test_scalar_throws_if_wrong_dtype_provided_for_str_types() -> None:
    with pytest.raises(ValueError, match='Cannot convert values'):
        sc.scalar(value='temp', unit=sc.units.one, dtype=sc.DType.float64)


def test_scalar_throws_UnitError_if_not_parsable() -> None:
    with pytest.raises(sc.UnitError):
        sc.scalar(value=1, unit='abcdef')


def test_scalar_of_numpy_array() -> None:
    value = np.array([1, 2, 3])
    with pytest.raises(sc.DimensionError):
        sc.scalar(value)
    var = sc.scalar(value, dtype=sc.DType.PyObject)
    assert var.dtype == sc.DType.PyObject
    np.testing.assert_array_equal(var.value, value)


def test_index_is_same_as_scalar_with_explicit_none_unit() -> None:
    assert sc.identical(sc.index(5), sc.scalar(5, unit=None))
    assert sc.identical(
        sc.index(6, dtype='int32'), sc.scalar(6, dtype='int32', unit=None)
    )


def test_index_unit_is_none() -> None:
    i = sc.index(5)
    assert i.unit is None


def test_index_raises_if_unit_given() -> None:
    with pytest.raises(TypeError, match='unexpected keyword argument'):
        sc.index(5, unit='')  # type: ignore[call-arg]


def test_zeros_creates_variable_with_correct_dims_and_shape() -> None:
    var = sc.zeros(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    expected = sc.Variable(dims=['x', 'y', 'z'], values=np.zeros([1, 2, 3]))
    assert sc.identical(var, expected)


def test_zeros_with_variances() -> None:
    shape = [1, 2, 3]
    var = sc.zeros(dims=['x', 'y', 'z'], shape=shape, with_variances=True)
    a = np.zeros(shape)
    expected = sc.Variable(dims=['x', 'y', 'z'], values=a, variances=a)
    assert sc.identical(var, expected)


def test_zeros_with_dtype_and_unit() -> None:
    var = sc.zeros(
        dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=sc.DType.int32, unit='m'
    )
    assert var.dtype == sc.DType.int32
    assert var.unit == 'm'


def test_zeros_dtypes() -> None:
    for dtype in (int, float, bool):
        assert sc.zeros(dims=(), shape=(), dtype=dtype).value == dtype(0)
    assert sc.zeros(
        dims=(), shape=(), unit='s', dtype='datetime64'
    ).value == np.datetime64(0, 's')
    assert sc.zeros(dims=(), shape=(), dtype=str).value == ''
    np.testing.assert_array_equal(
        sc.zeros(dims=(), shape=(), dtype=sc.DType.vector3).value, np.zeros(3)
    )
    np.testing.assert_array_equal(
        sc.zeros(dims=(), shape=(), dtype=sc.DType.linear_transform3).value,
        np.zeros((3, 3)),
    )


def test_zeros_float_default_unit_is_dimensionless() -> None:
    var = sc.zeros(dtype=float, dims=(), shape=())
    assert var.unit == sc.units.one


def test_zeros_string_default_unit_is_None() -> None:
    var = sc.zeros(dtype=str, dims=(), shape=())
    assert var.unit is None


def test_ones_float_default_unit_is_dimensionless() -> None:
    var = sc.ones(dtype=float, dims=(), shape=())
    assert var.unit == sc.units.one


def test_ones_creates_variable_with_correct_dims_and_shape() -> None:
    var = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    expected = sc.Variable(dims=['x', 'y', 'z'], values=np.ones([1, 2, 3]))
    assert sc.identical(var, expected)


def test_ones_with_variances() -> None:
    var = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3], with_variances=True)
    expected = sc.Variable(
        dims=['x', 'y', 'z'], values=np.ones([1, 2, 3]), variances=np.ones([1, 2, 3])
    )
    assert sc.identical(var, expected)


def test_ones_with_dtype_and_unit() -> None:
    var = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=sc.DType.int64, unit='s')
    assert var.dtype == sc.DType.int64
    assert var.unit == 's'


def test_ones_dtypes() -> None:
    for dtype in (int, float, bool):
        assert sc.ones(dims=(), shape=(), dtype=dtype).value == dtype(1)
    assert sc.ones(
        dims=(), shape=(), unit='s', dtype='datetime64'
    ).value == np.datetime64(1, 's')
    with pytest.raises(ValueError, match='string'):
        sc.ones(dims=(), shape=(), dtype=str)


def test_full_float_default_unit_is_dimensionless() -> None:
    var = sc.full(dims=(), shape=(), value=1.2)
    assert var.unit == sc.units.one


def test_full_string_default_unit_is_None() -> None:
    var = sc.full(dims=(), shape=(), value='abc')
    assert var.unit is None


def test_full_creates_variable_with_correct_dims_and_shape() -> None:
    var = sc.full(dims=['x', 'y', 'z'], shape=[1, 2, 3], value=12.34)
    expected = sc.Variable(dims=['x', 'y', 'z'], values=np.full([1, 2, 3], 12.34))
    assert sc.identical(var, expected)


def test_full_with_variances() -> None:
    var = sc.full(dims=['x', 'y', 'z'], shape=[1, 2, 3], value=12.34, variance=56.78)
    expected = sc.Variable(
        dims=['x', 'y', 'z'],
        values=np.full([1, 2, 3], 12.34),
        variances=np.full([1, 2, 3], 56.78),
    )
    assert sc.identical(var, expected)


def test_full_with_dtype_and_unit() -> None:
    var = sc.full(
        dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=sc.DType.int64, unit='s', value=1
    )
    assert var.dtype == sc.DType.int64
    assert var.unit == 's'


def test_full_with_int_value_gives_int64_dtype() -> None:
    var = sc.full(dims=(), shape=(), value=3)
    assert var.dtype == sc.DType.int64


def test_full_with_string_value_gives_string_dtype() -> None:
    var = sc.full(dims=(), shape=(), value='abc')
    assert var.dtype == str


def test_full_and_ones_equivalent() -> None:
    assert sc.identical(
        sc.full(dims=["x", "y"], shape=(2, 2), unit="m", value=1.0),
        sc.ones(dims=["x", "y"], shape=(2, 2), unit="m"),
    )


def test_full_and_zeros_equivalent() -> None:
    assert sc.identical(
        sc.full(dims=["x", "y"], shape=(2, 2), unit="m", value=0.0),
        sc.zeros(dims=["x", "y"], shape=(2, 2), unit="m"),
    )


def test_full_like() -> None:
    to_copy = sc.zeros(dims=["x", "y"], shape=(2, 2))

    assert sc.identical(
        sc.full_like(to_copy, value=123.45),
        sc.full(dims=["x", "y"], shape=(2, 2), value=123.45),
    )


@pytest.mark.parametrize('dtype', ['float64', 'int', 'bool'])
def test_full_like_with_dtype(dtype):
    to_copy = sc.zeros(dims=["x", "y"], shape=(2, 2))

    assert sc.identical(
        sc.full_like(to_copy, value=123.45, dtype=dtype),
        sc.full(
            dims=["x", "y"], shape=(2, 2), value=123.45, dtype=dtype, unit=to_copy.unit
        ),
    )


@pytest.mark.parametrize(('dims', 'shape'), [(['x'], (1,)), (['x', 'y'], (2, 3))])
def test_full_like_with_dims_shape(dims, shape):
    to_copy = sc.zeros(dims=["x", "y"], shape=(2, 2), unit='m')

    assert sc.identical(
        sc.full_like(to_copy, value=123.45, dims=dims, shape=shape),
        sc.full(dims=dims, shape=shape, value=123.45, unit='m'),
    )


@pytest.mark.parametrize('sizes', [{'x': 1}, {'x': 2, 'y': 3}])
def test_full_like_with_sizes(sizes) -> None:
    to_copy = sc.zeros(sizes=sizes, unit='m')

    assert sc.identical(
        sc.full_like(to_copy, value=123.45, sizes=sizes),
        sc.full(sizes=sizes, value=123.45, unit='m'),
    )


@pytest.mark.parametrize('unit', [None, '', 'm'])
def test_full_like_with_unit(unit):
    to_copy = sc.zeros(dims=["x", "y"], shape=(2, 2))

    assert sc.identical(
        sc.full_like(to_copy, value=123.45, unit=unit),
        sc.full(dims=["x", "y"], shape=(2, 2), value=123.45, unit=unit),
    )


def test_full_like_with_variance() -> None:
    to_copy = sc.zeros(dims=["x", "y"], shape=(2, 2))

    assert sc.identical(
        sc.full_like(to_copy, value=123.45, variance=67.89),
        sc.full(dims=["x", "y"], shape=(2, 2), value=123.45, variance=67.89),
    )


def test_empty_creates_variable_with_correct_dims_and_shape() -> None:
    var = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    expected = make_dummy(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    _compare_properties(var, expected)


def test_empty_with_variances() -> None:
    var = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3], with_variances=True)
    expected = make_dummy(dims=['x', 'y', 'z'], shape=[1, 2, 3], with_variances=True)
    _compare_properties(var, expected)


def test_empty_with_dtype_and_unit() -> None:
    var = sc.empty(
        dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=sc.DType.int32, unit='s'
    )
    assert var.dtype == sc.DType.int32
    assert var.unit == 's'


def test_empty_dtypes() -> None:
    for dtype in (int, float, bool):
        var = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=dtype)
        expected = make_dummy(dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=dtype)
        _compare_properties(var, expected)
    var = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype='datetime64')
    expected = sc.Variable(
        dims=['x', 'y', 'z'], values=np.full([1, 2, 3], 83), dtype='datetime64'
    )
    _compare_properties(var, expected)


def test_array_creates_correct_variable() -> None:
    dims = ['x']
    values = [1, 2, 3]
    variances = [4, 5, 6]
    unit = sc.units.m
    dtype = sc.DType.float64
    var = sc.array(
        dims=dims, values=values, variances=variances, unit=unit, dtype=dtype
    )
    expected = sc.Variable(
        dims=dims, values=values, variances=variances, unit=unit, dtype=dtype
    )

    assert sc.identical(var, expected)


def test_array_with_unit_None_gives_variable_with_unit_None() -> None:
    assert sc.array(dims=['x'], values=[1.2], unit=None).unit is None


def test_array_from_float_default_unit_is_dimensionless() -> None:
    assert sc.array(dims=['x'], values=[1.2]).unit == sc.units.one


def test_array_from_string_default_unit_is_None() -> None:
    assert sc.array(dims=['x'], values=['abc']).unit is None


def test_array_empty_dims() -> None:
    assert sc.identical(
        sc.array(dims=[], values=[1]), sc.scalar([1], dtype=sc.DType.PyObject)
    )
    a = np.asarray(1.1)
    assert sc.identical(sc.array(dims=None, values=a), sc.scalar(1.1))  # type: ignore[arg-type]
    assert sc.identical(sc.array(dims=[], values=a), sc.scalar(1.1))
    assert sc.identical(
        sc.array(dims=[], values=a, variances=a), sc.scalar(1.1, variance=1.1)
    )


@pytest.mark.parametrize('ndim', range(9))
def test_array_nd_contiguous_c_layout(ndim: int) -> None:
    shape = list(range(2, ndim + 2))
    values = np.arange(np.prod(shape)).reshape(shape)
    var = sc.array(dims=[f'dim_{d}' for d in range(ndim)], values=values)
    assert var.ndim == ndim
    assert var.shape == values.shape
    np.testing.assert_array_equal(var.values, values)


# ndim=0 doesn't work because np.asfortranarray(values) produces a 1d array
@pytest.mark.parametrize('ndim', range(1, 7))
def test_array_nd_contiguous_fortran_layout(ndim: int) -> None:
    shape = list(range(2, ndim + 2))
    values = np.arange(np.prod(shape)).reshape(shape)
    values = np.asfortranarray(values)
    var = sc.array(dims=[f'dim_{d}' for d in range(ndim)], values=values)
    assert var.ndim == ndim
    assert var.shape == values.shape
    np.testing.assert_array_equal(var.values, values)


def slice_array_in_dim(
    array: npt.NDArray[Any], dim: int, step: int
) -> npt.NDArray[Any]:
    # We cannot use numpy.take here because we don't want to
    # copy the output but get a sliced array with corresponding strides.
    if dim == 0:
        return array[::step]
    elif dim == 1:
        return array[:, ::step]
    elif dim == 2:
        return array[:, :, ::step]
    elif dim == 3:
        return array[:, :, :, ::step]
    elif dim == 4:
        return array[:, :, :, :, ::step]
    elif dim == 5:
        return array[:, :, :, :, :, ::step]
    elif dim == 6:
        return array[:, :, :, :, :, :, ::step]
    else:
        raise NotImplementedError()


@pytest.mark.parametrize(
    'ndim_and_slice_dim',
    [(ndim, slice_dim) for ndim in range(1, 7) for slice_dim in range(ndim)],
)
@pytest.mark.parametrize('step', [2, -1, -2])
def test_array_nd_sliced_c_layout(
    ndim_and_slice_dim: tuple[int, int], step: int
) -> None:
    ndim, slice_dim = ndim_and_slice_dim
    shape = list(range(2, ndim + 2))
    values = np.arange(np.prod(shape)).reshape(shape)
    values = slice_array_in_dim(values, slice_dim, step)
    var = sc.array(dims=[f'dim_{d}' for d in range(ndim)], values=values)
    assert var.ndim == ndim
    assert var.shape == values.shape
    np.testing.assert_array_equal(var.values, values)


@pytest.mark.parametrize(
    'ndim_and_slice_dims',
    [
        (ndim, slice_dim0, slice_dim1)
        for ndim in range(1, 7)
        for slice_dim0 in range(ndim)
        for slice_dim1 in range(ndim)
    ],
)
@pytest.mark.parametrize('step', [2, -1, -2])
def test_array_nd_sliced_twice_c_layout(
    ndim_and_slice_dims: tuple[int, int, int], step: int
) -> None:
    ndim, slice_dim0, slice_dim1 = ndim_and_slice_dims
    shape = list(range(2, ndim + 2))
    values = np.arange(np.prod(shape)).reshape(shape)
    values = slice_array_in_dim(
        slice_array_in_dim(values, slice_dim0, step), slice_dim1, step
    )
    var = sc.array(dims=[f'dim_{d}' for d in range(ndim)], values=values)
    assert var.ndim == ndim
    assert var.shape == values.shape
    np.testing.assert_array_equal(var.values, values)


@pytest.mark.parametrize(
    'ndim_and_slice_dim',
    [(ndim, slice_dim) for ndim in range(1, 7) for slice_dim in range(ndim)],
)
@pytest.mark.parametrize('step', [2, -1, -2])
def test_array_nd_sliced_fortran_layout(
    ndim_and_slice_dim: tuple[int, int], step: int
) -> None:
    ndim, slice_dim = ndim_and_slice_dim
    shape = list(range(2, ndim + 2))
    values = np.arange(np.prod(shape)).reshape(shape)
    values = np.asfortranarray(values)
    values = slice_array_in_dim(values, slice_dim, step)
    var = sc.array(dims=[f'dim_{d}' for d in range(ndim)], values=values)
    assert var.ndim == ndim
    assert var.shape == values.shape
    np.testing.assert_array_equal(var.values, values)


@pytest.mark.parametrize(
    'ndim_and_slice_dims',
    [
        (ndim, slice_dim0, slice_dim1)
        for ndim in range(1, 7)
        for slice_dim0 in range(ndim)
        for slice_dim1 in range(ndim)
    ],
)
@pytest.mark.parametrize('step', [2, -1, -2])
def test_array_nd_sliced_twice_fortran_layout(
    ndim_and_slice_dims: tuple[int, int, int], step: int
) -> None:
    ndim, slice_dim0, slice_dim1 = ndim_and_slice_dims
    shape = list(range(2, ndim + 2))
    values = np.arange(np.prod(shape)).reshape(shape)
    values = np.asfortranarray(values)
    values = slice_array_in_dim(
        slice_array_in_dim(values, slice_dim0, step), slice_dim1, step
    )
    var = sc.array(dims=[f'dim_{d}' for d in range(ndim)], values=values)
    assert var.ndim == ndim
    assert var.shape == values.shape
    np.testing.assert_array_equal(var.values, values)


@pytest.mark.parametrize('ndim', range(7, 9))
def test_array_nd_high_dim_sliced_begin_c_layout(ndim: int) -> None:
    shape = list(range(2, ndim + 2))
    values = np.arange(np.prod(shape)).reshape(shape)
    values = values[1:]
    var = sc.array(dims=[f'dim_{d}' for d in range(ndim)], values=values)
    assert var.ndim == ndim
    assert var.shape == values.shape
    np.testing.assert_array_equal(var.values, values)


@pytest.mark.parametrize('ndim', range(7, 9))
def test_array_nd_high_dim_sliced_end_c_layout(ndim: int) -> None:
    shape = list(range(2, ndim + 2))
    values = np.arange(np.prod(shape)).reshape(shape)
    values = values[:-1]
    var = sc.array(dims=[f'dim_{d}' for d in range(ndim)], values=values)
    assert var.ndim == ndim
    assert var.shape == values.shape
    np.testing.assert_array_equal(var.values, values)


def test_zeros_like() -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.zeros(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    zeros = sc.zeros_like(var)
    _compare_properties(zeros, expected)
    np.testing.assert_array_equal(zeros.values, 0)


@pytest.mark.parametrize('dtype', ['float64', 'int', 'bool'])
def test_zeros_like_with_dtype(dtype) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.zeros(
        dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=dtype, unit=var.unit
    )
    zeros = sc.zeros_like(var, dtype=dtype)
    _compare_properties(zeros, expected)
    np.testing.assert_array_equal(zeros.values, 0)


@pytest.mark.parametrize(('dims', 'shape'), [(['x'], (1,)), (['x', 'y'], (2, 3))])
def test_zeros_like_with_dims_shape(dims, shape) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.zeros(dims=dims, shape=shape)
    zeros = sc.zeros_like(var, dims=dims, shape=shape)
    _compare_properties(zeros, expected)
    np.testing.assert_array_equal(zeros.values, 0)


@pytest.mark.parametrize('sizes', [{'x': 1}, {'x': 2, 'y': 3}])
def test_zeros_like_with_sizes(sizes) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.zeros(sizes=sizes)
    zeros = sc.zeros_like(var, sizes=sizes)
    _compare_properties(zeros, expected)
    np.testing.assert_array_equal(zeros.values, 0)


@pytest.mark.parametrize('unit', [None, '', 'm'])
def test_zeros_like_with_unit(unit) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.zeros(dims=['x', 'y', 'z'], shape=[1, 2, 3], unit=unit)
    zeros = sc.zeros_like(var, unit=unit)
    _compare_properties(zeros, expected)
    np.testing.assert_array_equal(zeros.values, 0)


def test_zeros_like_with_variances_kwarg() -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.zeros(dims=['x', 'y', 'z'], shape=[1, 2, 3], with_variances=True)
    zeros = sc.zeros_like(var, with_variances=True)
    _compare_properties(zeros, expected)
    np.testing.assert_array_equal(zeros.values, 0)
    np.testing.assert_array_equal(zeros.variances, 0)


def test_zeros_like_with_variances() -> None:
    var = sc.Variable(
        dims=['x', 'y', 'z'],
        values=np.random.random([1, 2, 3]),
        variances=np.random.random([1, 2, 3]),
        unit='m',
        dtype=sc.DType.float32,
    )
    expected = sc.zeros(
        dims=['x', 'y', 'z'],
        shape=[1, 2, 3],
        with_variances=True,
        unit='m',
        dtype=sc.DType.float32,
    )
    zeros = sc.zeros_like(var)
    _compare_properties(zeros, expected)
    np.testing.assert_array_equal(zeros.values, 0)
    np.testing.assert_array_equal(zeros.variances, 0)


def test_ones_like() -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    ones = sc.ones_like(var)
    _compare_properties(sc.ones_like(var), expected)
    np.testing.assert_array_equal(ones.values, 1)


@pytest.mark.parametrize('dtype', ['float64', 'int', 'bool'])
def test_ones_like_with_dtype(dtype) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.ones(
        dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=dtype, unit=var.unit
    )
    ones = sc.ones_like(var, dtype=dtype)
    _compare_properties(ones, expected)
    np.testing.assert_array_equal(ones.values, 1)


@pytest.mark.parametrize(('dims', 'shape'), [(['x'], (1,)), (['x', 'y'], (2, 3))])
def test_ones_like_with_dims_shape(dims, shape) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.ones(dims=dims, shape=shape)
    ones = sc.ones_like(var, dims=dims, shape=shape)
    _compare_properties(ones, expected)
    np.testing.assert_array_equal(ones.values, 1)


@pytest.mark.parametrize('sizes', [{'x': 1}, {'x': 2, 'y': 3}])
def test_ones_like_with_sizes(sizes) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.ones(sizes=sizes)
    ones = sc.ones_like(var, sizes=sizes)
    _compare_properties(ones, expected)
    np.testing.assert_array_equal(ones.values, 1)


@pytest.mark.parametrize('unit', [None, '', 'm'])
def test_ones_like_with_unit(unit) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3], unit=unit)
    ones = sc.ones_like(var, unit=unit)
    _compare_properties(ones, expected)
    np.testing.assert_array_equal(ones.values, 1)


def test_ones_like_with_variances_kwarg() -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.ones(dims=['x', 'y', 'z'], shape=[1, 2, 3], with_variances=True)
    ones = sc.ones_like(var, with_variances=True)
    _compare_properties(ones, expected)
    np.testing.assert_array_equal(ones.values, 1)
    np.testing.assert_array_equal(ones.variances, 1)


def test_ones_like_with_variances() -> None:
    var = sc.Variable(
        dims=['x', 'y', 'z'],
        values=np.random.random([1, 2, 3]),
        variances=np.random.random([1, 2, 3]),
        unit='m',
        dtype=sc.DType.float32,
    )
    expected = sc.ones(
        dims=['x', 'y', 'z'],
        shape=[1, 2, 3],
        with_variances=True,
        unit='m',
        dtype=sc.DType.float32,
    )
    ones = sc.ones_like(var)
    _compare_properties(ones, expected)
    np.testing.assert_array_equal(ones.values, 1)
    np.testing.assert_array_equal(ones.variances, 1)


def test_empty_like() -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = make_dummy(dims=['x', 'y', 'z'], shape=[1, 2, 3])
    _compare_properties(sc.empty_like(var), expected)


@pytest.mark.parametrize('dtype', ['float64', 'int', 'bool'])
def test_empty_like_with_dtype(dtype) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.empty(
        dims=['x', 'y', 'z'], shape=[1, 2, 3], dtype=dtype, unit=var.unit
    )
    empty = sc.ones_like(var, dtype=dtype)
    _compare_properties(empty, expected)


@pytest.mark.parametrize(('dims', 'shape'), [(['x'], (1,)), (['x', 'y'], (2, 3))])
def test_empty_like_with_dims_shape(dims, shape) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.empty(dims=dims, shape=shape)
    empty = sc.ones_like(var, dims=dims, shape=shape)
    _compare_properties(empty, expected)


@pytest.mark.parametrize('sizes', [{'x': 1}, {'x': 2, 'y': 3}])
def test_empty_like_with_sizes(sizes) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.empty(sizes=sizes)
    empty = sc.empty_like(var, sizes=sizes)
    _compare_properties(empty, expected)


@pytest.mark.parametrize('unit', [None, '', 'm'])
def test_empty_like_with_unit(unit) -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3], unit=unit)
    empty = sc.ones_like(var, unit=unit)
    _compare_properties(empty, expected)


def test_empty_like_with_variances_kwarg() -> None:
    var = sc.Variable(dims=['x', 'y', 'z'], values=np.random.random([1, 2, 3]))
    expected = sc.empty(dims=['x', 'y', 'z'], shape=[1, 2, 3], with_variances=True)
    empty = sc.ones_like(var, with_variances=True)
    _compare_properties(empty, expected)


def test_empty_like_with_variances() -> None:
    var = sc.Variable(
        dims=['x', 'y', 'z'],
        values=np.random.random([1, 2, 3]),
        variances=np.random.random([1, 2, 3]),
        unit='m',
        dtype=sc.DType.float32,
    )
    expected = make_dummy(
        dims=['x', 'y', 'z'],
        shape=[1, 2, 3],
        with_variances=True,
        unit='m',
        dtype=sc.DType.float32,
    )
    _compare_properties(sc.empty_like(var), expected)


def test_linspace() -> None:
    values = np.linspace(1.2, 103.0, 51)
    var = sc.linspace('x', 1.2, 103.0, 51, unit='m', dtype=sc.DType.float32)
    expected = sc.Variable(dims=['x'], values=values, unit='m', dtype=sc.DType.float32)
    assert sc.identical(var, expected)


def test_linspace_none_unit() -> None:
    assert sc.linspace('x', 1.2, 103.0, 51, unit=None).unit is None


@pytest.mark.parametrize(
    'range_fns',
    [
        (sc.linspace, np.linspace),
        (sc.geomspace, np.geomspace),
        (sc.logspace, np.logspace),
    ],
    ids=('linspace', 'geomspace', 'logspace'),
)
def test_xyzspace_with_variables(
    range_fns: tuple[Callable[..., sc.Variable], Callable[..., npt.NDArray[Any]]],
) -> None:
    sc_fn, np_fn = range_fns
    var = sc_fn('x', sc.scalar(1.0), sc.scalar(5.0), 4)
    values = np_fn(1.0, 5.0, 4)
    assert sc.identical(var, sc.array(dims=['x'], values=values))


@pytest.mark.parametrize(
    'range_fns',
    [(sc.linspace, np.linspace), (sc.geomspace, np.geomspace)],
    ids=('linspace', 'geomspace'),
)
def test_xyzspace_with_variables_set_unit(
    range_fns: tuple[Callable[..., sc.Variable], Callable[..., npt.NDArray[Any]]],
) -> None:
    sc_fn, np_fn = range_fns
    var = sc_fn(
        'x', sc.scalar(1.0, unit='m'), sc.scalar(5000.0, unit='mm'), 4, unit='m'
    )
    values = np_fn(1.0, 5.0, 4)
    assert sc.identical(var, sc.array(dims=['x'], values=values, unit='m'))


def test_logspace_with_variables_set_unit() -> None:
    assert sc.identical(
        sc.logspace('x', sc.scalar(1.0), sc.scalar(3.0), 3, unit='m'),
        sc.array(dims=['x'], values=[10.0, 100.0, 1000.0], unit='m'),
    )


@pytest.mark.parametrize('unit', [sc.units.default_unit, 'one', 'm'])
def test_logspace_with_variables_input_must_be_dimensionless(
    unit: sc.Unit | str,
) -> None:
    with pytest.raises(sc.UnitError):
        sc.logspace('x', sc.scalar(1.0), sc.scalar(3.0, unit='m'), 4, unit=unit)
    with pytest.raises(sc.UnitError):
        sc.logspace('x', sc.scalar(1.0, unit='m'), sc.scalar(3.0), 4, unit=unit)
    with pytest.raises(sc.UnitError):
        sc.logspace(
            'x', sc.scalar(1.0, unit='m'), sc.scalar(3.0, unit='m'), 4, unit=unit
        )


@pytest.mark.parametrize(
    'range_fns',
    [
        (sc.linspace, np.linspace),
        (sc.geomspace, np.geomspace),
        (sc.logspace, np.logspace),
    ],
    ids=('linspace', 'geomspace', 'logspace'),
)
def test_xyzspace_with_variables_num_can_be_int_variable(
    range_fns: tuple[Callable[..., sc.Variable], Callable[..., npt.NDArray[Any]]],
) -> None:
    sc_fn, np_fn = range_fns
    var = sc_fn('x', sc.scalar(1.0), sc.scalar(5.0), sc.scalar(4))
    values = np_fn(1.0, 5.0, 4)
    assert sc.identical(var, sc.array(dims=['x'], values=values))


@pytest.mark.parametrize(
    'range_fn',
    [sc.linspace, sc.geomspace, sc.logspace],
    ids=('linspace', 'geomspace', 'logspace'),
)
@pytest.mark.parametrize('num', [3.0, sc.scalar(3.0)])
def test_xyzspace_with_variables_num_cannot_be_float_variable(
    range_fn: Callable[..., sc.Variable],
    num: float | sc.Variable,
) -> None:
    start = sc.scalar(1)
    stop = sc.scalar(3)
    with pytest.raises(TypeError):
        range_fn('x', start, stop, num)


def test_logspace() -> None:
    values = np.logspace(2.0, 3.0, num=4)
    var = sc.logspace('y', 2.0, 3.0, num=4, unit='s')
    expected = sc.Variable(dims=['y'], values=values, unit='s', dtype=sc.DType.float64)
    assert sc.identical(var, expected)


def test_geomspace() -> None:
    values = np.geomspace(1, 1000, num=4)
    var = sc.geomspace('z', 1, 1000, num=4)
    expected = sc.Variable(dims=['z'], values=values, dtype=sc.DType.float64)
    assert sc.identical(var, expected)


def test_arange() -> None:
    values = np.arange(21)
    var = sc.arange('x', 21, unit='m', dtype=sc.DType.int32)
    expected = sc.Variable(dims=['x'], values=values, unit='m', dtype=sc.DType.int32)
    assert sc.identical(var, expected)
    values = np.arange(10, 21, 2)
    var = sc.arange(dim='x', start=10, stop=21, step=2, unit='m', dtype=sc.DType.int32)
    expected = sc.Variable(dims=['x'], values=values, unit='m', dtype=sc.DType.int32)
    assert sc.identical(var, expected)


def test_arange_datetime_from_int() -> None:
    var = sc.arange('t', 14, 65, 21, unit='s', dtype='datetime64')
    expected = sc.datetimes(dims=['t'], values=[14, 35, 56], unit='s')
    assert sc.identical(var, expected)


def test_arange_datetime_from_np_datetime64() -> None:
    var = sc.arange(
        't', np.datetime64('2022-08-02T11:18'), np.datetime64('2022-08-02T11:52'), 15
    )
    expected = sc.datetimes(
        dims=['t'],
        values=['2022-08-02T11:18', '2022-08-02T11:33', '2022-08-02T11:48'],
        unit='min',
    )
    assert sc.identical(var, expected)


def test_arange_datetime_from_str_raises_if_step_has_no_unit() -> None:
    with pytest.raises(TypeError):
        sc.arange(
            't', '2022-08-02T06:42:45', '2022-08-02T06:43:33', 16, dtype='datetime64'
        )


def test_arange_datetime_from_str_raises_given_string_with_timezone() -> None:
    with pytest.raises(raise_np_datetime, match='timezone'):
        sc.arange(
            't',
            '2022-08-02T06:42:45Z',
            '2022-08-02T06:43:33Z',
            16 * sc.Unit('s'),
            dtype='datetime64',
        )


def test_arange_datetime_from_str() -> None:
    var = sc.arange(
        't',
        '2022-08-02T06:42:45',
        '2022-08-02T06:43:33',
        16 * sc.Unit('s'),
        dtype='datetime64',
    )
    expected = sc.datetimes(
        dims=['t'],
        values=['2022-08-02T06:42:45', '2022-08-02T06:43:01', '2022-08-02T06:43:17'],
        unit='s',
    )
    assert sc.identical(var, expected)


def test_arange_datetime_from_scipp_datetime() -> None:
    var = sc.arange(
        't',
        sc.datetime('2013-04-25T14:09:11'),
        sc.datetime('2013-04-25T14:11:23'),
        sc.scalar(62, unit='s'),
    )
    expected = sc.datetimes(
        dims=['t'],
        values=['2013-04-25T14:09:11', '2013-04-25T14:10:13', '2013-04-25T14:11:15'],
        unit='s',
    )
    assert sc.identical(var, expected)


@pytest.mark.parametrize('unit', ['one', sc.units.default_unit])
def test_arange_with_variables(unit: str | sc.Unit) -> None:
    start = sc.scalar(1)
    stop = sc.scalar(4)
    step = sc.scalar(1)
    assert sc.identical(
        sc.arange('x', start, stop, step, unit=unit),
        sc.array(dims=['x'], values=[1, 2, 3], unit='one', dtype='int64'),
    )
    assert sc.identical(
        sc.arange('x', start, stop, unit=unit),
        sc.array(dims=['x'], values=[1, 2, 3], unit='one', dtype='int64'),
    )
    assert sc.identical(
        sc.arange('x', stop, unit=unit),
        sc.array(dims=['x'], values=[0, 1, 2, 3], unit='one', dtype='int64'),
    )


def test_arange_with_variables_uses_units_of_args() -> None:
    start = sc.scalar(10.0, unit='s')
    stop = sc.scalar(33.0, unit='s')
    step = sc.scalar(10.0, unit='s')
    assert sc.identical(
        sc.arange('x', start, stop, step),
        sc.array(dims=['x'], values=[10.0, 20.0, 30.0], unit='s'),
    )
    assert sc.identical(
        sc.arange('x', start, stop),
        sc.array(dims=['x'], values=np.arange(10.0, 33.0, 1.0), unit='s'),
    )
    assert sc.identical(
        sc.arange('x', stop), sc.array(dims=['x'], values=np.arange(33.0), unit='s')
    )


def test_arange_with_variables_without_unit_arg_requires_same_unit() -> None:
    start = sc.scalar(1, unit='m')
    stop = sc.scalar(4, unit='m')
    step = sc.scalar(1, unit='m')
    with pytest.raises(sc.UnitError):
        sc.arange('x', start, stop, sc.scalar(500, unit='mm'))
    with pytest.raises(sc.UnitError):
        sc.arange('x', start, sc.scalar(4000, unit='mm'), step)
    with pytest.raises(sc.UnitError):
        sc.arange('x', sc.scalar(0.001, unit='km'), stop, step)


def test_arange_with_variables_set_unit() -> None:
    start = sc.scalar(1, unit='m')
    stop = sc.scalar(4, unit='m')
    step = sc.scalar(1, unit='m')
    unit = 'm'
    assert sc.identical(
        sc.arange('x', start, stop, step, unit=unit),
        sc.array(dims=['x'], values=[1, 2, 3], unit='m', dtype='int64'),
    )
    assert sc.identical(
        sc.arange('x', start, stop, unit=unit),
        sc.array(dims=['x'], values=[1, 2, 3], unit='m', dtype='int64'),
    )

    assert sc.identical(
        sc.arange('x', start, stop, step, unit='mm'),
        sc.array(dims=['x'], values=[1000, 2000, 3000], unit='mm', dtype='int64'),
    )
    assert sc.identical(
        sc.arange('x', start, stop, unit='cm'),
        sc.array(dims=['x'], values=np.arange(100, 400, 1), unit='cm', dtype='int64'),
    )

    assert sc.identical(
        sc.arange('x', start, stop, sc.scalar(500, unit='mm'), unit='mm'),
        sc.array(
            dims=['x'],
            values=[1000, 1500, 2000, 2500, 3000, 3500],
            unit='mm',
            dtype='int64',
        ),
    )
    assert sc.identical(
        sc.arange('x', start, stop, sc.scalar(500.0, unit='mm'), unit='m'),
        sc.array(dims=['x'], values=[1, 1.5, 2, 2.5, 3, 3.5], unit='m'),
    )
    # All args are integers -> truncates step.
    assert sc.identical(
        sc.arange('x', start, stop, sc.scalar(500, unit='mm'), unit='m'),
        sc.array(dims=['x'], values=[1, 2, 3], unit='m', dtype='int64'),
    )


def test_arange_with_variables_set_unit_must_be_convertible() -> None:
    start = sc.scalar(1, unit='m')
    stop = sc.scalar(4, unit='m')
    step = sc.scalar(1, unit='m')
    with pytest.raises(sc.UnitError):
        sc.arange('x', start, stop, step, unit='kg')
    with pytest.raises(sc.UnitError):
        sc.arange('x', start, stop, unit='kg')
    with pytest.raises(sc.UnitError):
        sc.arange('x', stop, unit='kg')


def test_arange_with_variables_mixed_types_not_allowed() -> None:
    start = sc.scalar(1, unit='m')
    stop = 4
    step = sc.scalar(1, unit='m')
    unit = 'm'
    with pytest.raises(TypeError):
        sc.arange('x', start, stop, step)  # type: ignore[type-var]
    with pytest.raises(TypeError):
        sc.arange('x', start, stop, step, unit=unit)  # type: ignore[type-var]


def test_arange_with_variables_requires_scalar() -> None:
    with pytest.raises(sc.DimensionError):
        sc.arange('x', sc.array(dims=['x'], values=[1, 2]))
    with pytest.raises(sc.DimensionError):
        sc.arange('x', sc.scalar(1), sc.array(dims=['x'], values=[1, 2]))


def test_arange_with_variables_does_not_allow_variances() -> None:
    start = sc.scalar(1)
    stop = sc.scalar(4)
    step = sc.scalar(1)
    with pytest.raises(sc.VariancesError):
        sc.arange('x', start, stop, sc.scalar(1.0, variance=0.1))
    with pytest.raises(sc.VariancesError):
        sc.arange('x', start, sc.scalar(4.0, variance=0.1), step)
    with pytest.raises(sc.VariancesError):
        sc.arange('x', sc.scalar(1.0, variance=0.1), stop, step)


def test_arange_with_variables_mixed_dtype() -> None:
    assert sc.identical(
        sc.arange('x', sc.scalar(1), sc.scalar(4.0), sc.scalar(1)),
        sc.array(dims=['x'], values=[1.0, 2.0, 3.0], dtype='float64'),
    )
    assert sc.identical(
        sc.arange('x', sc.scalar(1), sc.scalar(4.0), sc.scalar(1), dtype='int64'),
        sc.array(dims=['x'], values=[1, 2, 3], dtype='int64'),
    )


@pytest.mark.parametrize('dtype', [np.int32, np.int64, np.float32, np.float64])
def test_arange_with_uniform_numpy_arg_dtype_creates_array_with_same_dtype(
    dtype: type,
) -> None:
    assert sc.identical(
        sc.arange('x', dtype(2)), sc.array(dims=['x'], values=[0, 1], dtype=dtype)
    )
    assert sc.identical(
        sc.arange('x', dtype(2), dtype(4)),
        sc.array(dims=['x'], values=[2, 3], dtype=dtype),
    )
    assert sc.identical(
        sc.arange('x', dtype(2), dtype(4), dtype(2)),
        sc.array(dims=['x'], values=[2], dtype=dtype),
    )


@pytest.mark.parametrize('dtype', [np.int32, np.int64, np.float32, np.float64])
def test_arange_with_uniform_scipp_arg_dtype_creates_array_with_same_dtype(
    dtype: type,
) -> None:
    assert sc.identical(
        sc.arange('x', sc.scalar(2, dtype=dtype)),
        sc.array(dims=['x'], values=[0, 1], dtype=dtype),
    )
    assert sc.identical(
        sc.arange('x', sc.scalar(2, dtype=dtype), sc.scalar(4, dtype=dtype)),
        sc.array(dims=['x'], values=[2, 3], dtype=dtype),
    )
    assert sc.identical(
        sc.arange(
            'x',
            sc.scalar(2, dtype=dtype),
            sc.scalar(4, dtype=dtype),
            sc.scalar(2, dtype=dtype),
        ),
        sc.array(dims=['x'], values=[2], dtype=dtype),
    )


@pytest.mark.parametrize(
    ('dtype', 'larger'),
    [
        (np.int32, np.int64),
        (np.float32, np.float64),
        (np.int32, np.float64),
        (np.int64, np.float64),
    ],
)
def test_arange_with_non_uniform_arg_dtype_creates_array_with_larger_dtype(
    dtype: type, larger: type
) -> None:
    assert sc.identical(
        sc.arange('x', dtype(2), larger(4)),
        sc.array(dims=['x'], values=[2, 3], dtype=larger),
    )
    assert sc.identical(
        sc.arange('x', larger(2), dtype(4)),
        sc.array(dims=['x'], values=[2, 3], dtype=larger),
    )
    assert sc.identical(
        sc.arange('x', larger(2), dtype(4), dtype(2)),
        sc.array(dims=['x'], values=[2], dtype=larger),
    )
    assert sc.identical(
        sc.arange('x', dtype(2), larger(4), dtype(2)),
        sc.array(dims=['x'], values=[2], dtype=larger),
    )
    assert sc.identical(
        sc.arange('x', dtype(2), dtype(4), larger(2)),
        sc.array(dims=['x'], values=[2], dtype=larger),
    )


def test_zeros_sizes() -> None:
    dims = ['x', 'y', 'z']
    shape = [2, 3, 4]
    assert sc.identical(
        sc.zeros(dims=dims, shape=shape),
        sc.zeros(sizes=dict(zip(dims, shape, strict=True))),
    )
    with pytest.raises(ValueError, match='dims and shape must both be None'):
        sc.zeros(dims=dims, shape=shape, sizes=dict(zip(dims, shape, strict=True)))


def test_ones_sizes() -> None:
    dims = ['x', 'y', 'z']
    shape = [2, 3, 4]
    assert sc.identical(
        sc.ones(dims=dims, shape=shape),
        sc.ones(sizes=dict(zip(dims, shape, strict=True))),
    )
    with pytest.raises(ValueError, match='dims and shape must both be None'):
        sc.ones(dims=dims, shape=shape, sizes=dict(zip(dims, shape, strict=True)))


def test_empty_sizes() -> None:
    dims = ['x', 'y', 'z']
    shape = [2, 3, 4]
    _compare_properties(
        sc.empty(dims=dims, shape=shape),
        sc.empty(sizes=dict(zip(dims, shape, strict=True))),
    )
    with pytest.raises(ValueError, match='dims and shape must both be None'):
        sc.empty(dims=dims, shape=shape, sizes=dict(zip(dims, shape, strict=True)))


@pytest.mark.parametrize('timezone', ['Z', '-05:00', '+02'])
def test_datetime_raises_given_string_with_timezone(timezone: str) -> None:
    with pytest.raises(raise_np_datetime, match='timezone'):
        sc.datetime(f'2152-11-25T13:13:46{timezone}')


@pytest.mark.parametrize('timezone', ['Z', '-05:00', '+02'])
def test_datetimes_raises_given_string_with_timezone(timezone: str) -> None:
    with pytest.raises(raise_np_datetime, match='timezone'):
        sc.datetimes(dims=['time'], values=[f'2152-11-25T13:13:46{timezone}'], unit='s')


def test_datetime() -> None:
    assert sc.identical(
        sc.datetime('1970', unit='Y'), sc.scalar(np.datetime64('1970', 'Y'))
    )
    assert sc.identical(
        sc.datetime('2015-06-13'), sc.scalar(np.datetime64('2015-06-13', 'D'))
    )
    assert sc.identical(
        sc.datetime('2152-11-25T13:13:46'),
        sc.scalar(np.datetime64('2152-11-25T13:13:46', 's')),
    )
    assert sc.identical(
        sc.datetime('2152-11-25T13:13:46', unit='h'),
        sc.scalar(np.datetime64('2152-11-25T13', 'h')),
    )
    assert sc.identical(
        sc.datetime('2152-11-25T13:13:46', unit='us'),
        sc.scalar(np.datetime64('2152-11-25T13:13:46', 'us')),
    )

    assert sc.identical(
        sc.datetime(626, unit='s'), sc.scalar(626, dtype='datetime64', unit='s')
    )
    assert sc.identical(
        sc.datetime(2**10, unit='ns'),
        sc.scalar(2**10, dtype='datetime64', unit='ns'),
    )
    assert sc.identical(
        sc.datetime(-94716, unit='min'),
        sc.scalar(-94716, dtype='datetime64', unit='min'),
    )

    assert sc.identical(
        sc.datetime(np.datetime64(314, 's'), unit='s'),
        sc.scalar(314, dtype='datetime64', unit='s'),
    )
    assert sc.identical(
        sc.datetime(np.datetime64(671, 'h')),
        sc.scalar(671, dtype='datetime64', unit='h'),
    )


def test_datetimes() -> None:
    assert sc.identical(
        sc.datetimes(dims=['t'], values=['1970', '2021'], unit='Y'),
        sc.array(
            dims=['t'],
            values=[np.datetime64('1970', 'Y'), np.datetime64('2021', 'Y')],
            unit='Y',
        ),
    )
    assert sc.identical(
        sc.datetimes(
            dims=['t'], values=['2152-11-25T13:13:46', '1111-11-11T11:11:11'], unit='s'
        ),
        sc.array(
            dims=['t'],
            values=[
                np.datetime64('2152-11-25T13:13:46', 's'),
                np.datetime64('1111-11-11T11:11:11', 's'),
            ],
            unit='s',
        ),
    )
    assert sc.identical(
        sc.datetimes(
            dims=['t'], values=['2152-11-25T13:13:46', '1111-11-11T11:11:11'], unit='us'
        ),
        sc.array(
            dims=['t'],
            values=[
                np.datetime64('2152-11-25T13:13:46', 'us'),
                np.datetime64('1111-11-11T11:11:11', 'us'),
            ],
            unit='us',
        ),
    )

    assert sc.identical(
        sc.datetimes(dims=['t'], values=[0, 123, 2**10], unit='s'),
        sc.array(dims=['t'], values=np.array([0, 123, 2**10], dtype='datetime64[s]')),
    )
    assert sc.identical(
        sc.datetimes(dims=['t'], values=[-723, 2**13, -(3**5)], unit='min'),
        sc.array(
            dims=['t'],
            values=np.array([-723, 2**13, -(3**5)], dtype='datetime64[m]'),
        ),
    )


def test_datetimes_raises_if_given_invalid_unit_string() -> None:
    with pytest.raises(sc.UnitError):
        sc.datetimes(
            dims=['t'],
            values=['2022-08-02T11:18', '2022-08-02T11:33'],
            unit='not-a-valid-unit',
        )


def test_datetime_raises_if_given_invalid_unit_string() -> None:
    with pytest.raises(sc.UnitError):
        sc.datetime('2022-08-02T11:18', unit='not-a-valid-unit')


def test_datetimes_raises_if_given_unit_m() -> None:
    # This one is special because in NumPy, 'm' means minute.
    with pytest.raises(sc.UnitError):
        sc.datetimes(
            dims=['t'], values=['2022-08-02T11:18', '2022-08-02T11:33'], unit='m'
        )


def test_datetime_raises_if_given_unit_m() -> None:
    # This one is special because in NumPy, 'm' means minute.
    with pytest.raises(sc.UnitError):
        sc.datetime('2022-08-02T11:18', unit='m')


def test_datetime_epoch() -> None:
    assert sc.identical(
        sc.epoch(unit='s'), sc.scalar(np.datetime64('1970-01-01T00:00:00', 's'))
    )
    assert sc.identical(sc.epoch(unit='D'), sc.scalar(np.datetime64('1970-01-01', 'D')))
