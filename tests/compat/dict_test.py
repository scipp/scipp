# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from collections.abc import Callable
from inspect import getmembers
from typing import Any, Literal

import numpy as np
import pytest

import scipp as sc
from scipp import spatial

_ALL_DTYPES: dict[str, sc.DType] = dict(
    getmembers(sc.DType, lambda m: isinstance(m, sc.DType))
)
_NORMAL_DTYPES = sorted(
    set(_ALL_DTYPES)
    - {
        'DataArray',
        'DataArrayView',
        'Dataset',
        'DatasetView',
        'Variable',
        'VariableView',
    }
)


def _get_dtype_hint(dtype: str, unit: str | None = None) -> Any:
    """Get the object suitable for being passed to the ``dtype``
    argument of NumPy constructors.

    Note
    ----
    Datetimes need a concrete unit, or :attr:`Variable.values` will
    error out trying to come up with one.
    """
    if dtype == 'datetime64':
        assert unit is not None
        return f'datetime64[{unit}]'
    if dtype.startswith(('int', 'float', 'uint')):  # Numeric types
        return getattr(np, dtype)
    if dtype.endswith('3'):  # Linalg types
        return float
    return {
        'string': str,
        'bool': bool,
    }.get(dtype, object)  # Fallback


def _make_empty_variable_kwargs(
    dtype: str,
    shape: tuple[int, ...],
    unit: str | None = None,
) -> tuple[Callable[..., sc.Variable], bool, list[str], dict[str, Any]]:
    """Get the keyword args for creatign a :class:`Variable` of size
    zero of the required data type.

    Returns
    -------
    make_var: Callable[..., Variable]
        Callable to pass ``kwargs`` to, returning a :class:`Variable`.
    fold: bool
        Whether the result `Variable` is to be :meth:`.fold`-ed into the
        wanted shape.
    dims: list[str]
        The :attr:`.dims` of the `Variable` (post-folding where
        appropriate).
    kwargs: dict[str, Any]
        Arguments to be passed to ``kwargs``.

    Note
    ----
    If ``fold`` is true, the dim ``'_'`` is to be folded into ``dims``.

    See also
    --------
    :func:`_make_empty_variable`
    """
    make_var: Callable[..., sc.Variable]
    dims = [f'dim{i}' for i, _ in enumerate(shape, 1)]
    make_var = {  # type: ignore[assignment]
        "int64": sc.array,
        "int32": sc.array,
        "vector3": sc.vectors,
        "affine_transform3": spatial.affine_transforms,
        "linear_transform3": spatial.linear_transforms,
        "translation3": spatial.translations,
        "rotation3": spatial.rotations,
    }.get(dtype, sc.array)

    if dtype == 'datetime64':
        # See note on units at `_get_dtype_hint()`
        assert unit is not None
        np_dtype = _get_dtype_hint(dtype, unit)
        unit = None
    else:
        np_dtype = _get_dtype_hint(dtype)

    fold = dtype in ('string', 'PyObject')
    if fold:
        # Note: instantiating from N-D arrays is sketchy for non-native
        # dtypes; just `.fold()` it post-hoc.
        kwargs = {
            "values": np.array([], dtype=np_dtype),
            "dims": '_',  # To be massaged into `dims` by folding
        }
    else:
        # Take care of extra dimensions in linalg objects
        np_shape = shape + {
            "vector3": (3,),
            "affine_transform3": (4, 4),
            "linear_transform3": (3, 3),
            "translation3": (3,),
            "rotation3": (4,),
        }.get(dtype, ())
        kwargs = {
            "values": np.array([], dtype=np_dtype).reshape(np_shape),
            "dims": dims,
        }
    if unit is not None:
        kwargs["unit"] = unit
    if dtype.startswith(('int', 'float', 'uint')):
        kwargs["dtype"] = _ALL_DTYPES[dtype]

    return make_var, fold, dims, kwargs


def _make_empty_variable(
    dtype: str,
    shape: tuple[int, ...],
    unit: str | None = None,
) -> sc.Variable:
    """Thin wrapper around :func:`_make_empty_variable_kwargs`, using
    its result to actually create the :class:`Variable`.
    """
    make_var, fold, dims, kwargs = _make_empty_variable_kwargs(
        dtype,
        shape,
        unit,
    )
    var = make_var(**kwargs)
    if fold:
        var = var.fold('_', dims=dims, shape=shape)

    assert var.shape == shape
    assert var.dtype == _ALL_DTYPES[dtype]
    return var


@pytest.mark.parametrize('ndim', [1, 2])
def test_variable_to_dict(ndim: Literal[1, 2]) -> None:
    var = sc.Variable(
        dims=["x"],
        values=np.arange(10.0),
        variances=np.random.random(10),
        unit=sc.units.m,
    )
    if ndim > 1:
        var = var.fold('x', sizes={'x': 2, 'y': 5})
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == var.dims
    assert var_dict["shape"] == var.shape
    assert np.array_equal(var_dict["values"], var.values)
    assert np.array_equal(var_dict["variances"], var.variances)
    assert var_dict["unit"] == var.unit
    assert var_dict["dtype"] == var.dtype


@pytest.mark.parametrize('dtype', _NORMAL_DTYPES)
@pytest.mark.parametrize(
    'shape',
    ['0', '0,0', '2,0', '0,2', '0,0,0', '2,0,0', '0,2,0', '0,0,2'],
)
def test_empty_variable_to_dict(dtype: str, shape: str) -> None:
    shape_tuple: tuple[int, ...] = tuple(int(s) for s in shape.split(','))
    unit: str | None = {
        'int64': '1E5 s',
        'vector3': 'm',
        'translation3': '5',
        'datetime64': 's',
    }.get(dtype)
    var = _make_empty_variable(dtype, shape_tuple, unit)

    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == var.dims
    assert var_dict["shape"] == var.shape
    assert np.array_equal(var_dict["values"], var.values)
    assert var_dict["dtype"] == var.dtype
    if unit is None:
        expected_unit = {
            'PyObject': None,
            'string': None,
            'bool': None,
        }.get(dtype, sc.Unit('1'))
        assert var_dict.get('unit') == expected_unit
    else:
        assert var_dict["unit"] == sc.Unit(unit)


@pytest.mark.parametrize(
    'dtype',
    ['float64', 'float32'],  # Note: only floats can have vars
)
@pytest.mark.parametrize(
    'shape',
    ['0', '0,0', '2,0', '0,2', '0,0,0', '2,0,0', '0,2,0', '0,0,2'],
)
@pytest.mark.parametrize('has_var', [True, False])
def test_empty_variable_variances_to_dict(
    dtype: str,
    shape: str,
    has_var: bool,
) -> None:
    shape_tuple: tuple[int, ...] = tuple(int(s) for s in shape.split(','))
    var = _make_empty_variable(dtype, (0,))
    if has_var:
        var.variances = []
    var_dict = sc.to_dict(
        var.fold(
            var.dims[0],
            dims=[f'dim{i}' for i, _ in enumerate(shape_tuple, 1)],
            shape=shape_tuple,
        )
    )

    if not has_var:
        assert var_dict.get('variances') is None
        return
    variances = var_dict['variances']
    assert variances.shape == shape_tuple
    assert not variances.size


def test_variable_0D_to_dict() -> None:
    var = 12.0 * sc.units.one
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == ()
    assert var_dict["shape"] == ()
    assert var_dict["values"] == 12.0


@pytest.mark.parametrize('ndim', [1, 2])
@pytest.mark.parametrize('dtype', ['string', 'PyObject'])
def test_variable_nonnative_to_dict(
    ndim: Literal[1, 2],
    dtype: Literal['string', 'PyObject'],
) -> None:
    data = np.random.choice(['foo', 'bar'], 10)
    if dtype == 'PyObject':
        data = data.astype(object)
    var = sc.Variable(dims=('x',), values=data)
    if ndim > 1:
        var = var.fold('x', sizes={'x': 2, 'y': 5})
        dims: tuple[str, ...] = ('x', 'y')
        shape: tuple[int, ...] = (2, 5)
    else:
        dims, shape = ('x',), (10,)
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == var.dims == dims
    assert var_dict["shape"] == var.shape == shape
    # Note: we've flattened the internal representation
    assert np.array_equal(var_dict["values"], data)
    assert var_dict["dtype"] == var.dtype == getattr(sc.DType, dtype)


@pytest.mark.parametrize('dtype', ['string', 'PyObject'])
def test_variable_0D_nonnative_to_dict(
    dtype: Literal['string', 'PyObject'],
) -> None:
    dtype_obj = getattr(sc.DType, dtype)
    var = sc.Variable(values='foo', dims=(), dtype=dtype_obj)
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == ()
    assert var_dict["shape"] == ()
    assert np.array_equal(var_dict["values"], 'foo')
    assert var_dict["dtype"] == dtype_obj


@pytest.mark.parametrize('ndim', [1, 2])
def test_variable_vector_to_dict(ndim: Literal[1, 2]) -> None:
    var = sc.vectors(dims=['x'], values=np.random.random([10, 3]))
    dims: tuple[str, ...]
    shape: tuple[int, ...]
    if ndim > 1:
        var = var.fold("x", sizes={"x": 2, "y": 5})
        dims, shape = ('x', 'y'), (2, 5)
    else:
        dims, shape = ('x',), (10,)
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == dims
    assert var_dict["shape"] == shape
    assert var_dict["values"].shape == (*shape, 3)
    assert var_dict["dtype"] == sc.DType.vector3


def test_variable_0D_vector_to_dict() -> None:
    var = sc.vector(value=[1, 2, 3])
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == ()
    assert var_dict["shape"] == ()
    assert np.array_equal(var_dict["values"], [1, 2, 3])
    assert var_dict["dtype"] == sc.DType.vector3


@pytest.mark.parametrize('ndim', [1, 2])
@pytest.mark.parametrize('transform', ['linear', 'affine'])
def test_variable_transform_to_dict(
    ndim: Literal[1, 2],
    transform: Literal['linear', 'affine'],
) -> None:
    data = np.array(
        [
            np.arange(9.0).reshape(3, 3),
            np.arange(5.0, 14.0).reshape(3, 3),
            np.arange(1.0, 10.0).reshape(3, 3),
            np.arange(2.0, 11.0).reshape(3, 3),
        ]
    )
    var = sc.spatial.linear_transforms(dims=['x'], values=data, unit=sc.units.m)
    if transform == 'affine':
        var = var * sc.spatial.translation(value=np.array([3, 2, 1]), unit='m')
        elem_shape, dtype = (4, 4), sc.DType.affine_transform3
    else:
        elem_shape, dtype = (3, 3), sc.DType.linear_transform3

    if ndim > 1:
        var = var.fold('x', sizes={'x': 2, 'y': 2})
        shape: tuple[int, ...] = (2, 2)
    else:
        shape = (4,)
    var_dict = sc.to_dict(var)
    assert var_dict["shape"] == shape
    assert var_dict["values"].shape == (*shape, *elem_shape)
    assert var_dict["dtype"] == dtype


@pytest.mark.parametrize('transform', ['linear', 'affine'])
def test_variable_0D_transform_to_dict(
    transform: Literal['linear', 'affine'],
) -> None:
    var = sc.spatial.linear_transform(value=np.arange(1, 10).reshape(3, 3))
    if transform == 'affine':
        var = var * sc.spatial.translation(value=np.array([3, 2, 1]), unit='m')
        dtype = sc.DType.affine_transform3
    else:
        dtype = sc.DType.linear_transform3
    var_dict = sc.to_dict(var)
    assert var_dict["dims"] == ()
    assert var_dict["shape"] == ()
    assert np.array_equal(
        var_dict["values"][:3, :3],
        [[1, 2, 3], [4, 5, 6], [7, 8, 9]],
    )
    assert var_dict["dtype"] == dtype


@pytest.mark.parametrize('ndim', [1, 2])
def test_variable_from_dict(ndim: Literal[1, 2]) -> None:
    if ndim > 1:
        dims: tuple[str, ...] = ('x', 'y')
        shape: tuple[int, ...] = (2, 5)
    else:
        dims, shape = ('x',), (10,)
    var_dict = {
        "dims": dims,
        "values": np.random.random(shape),
        "variances": np.random.random(shape),
    }
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == var_dict["dims"] == dims
    assert var.shape == shape
    assert np.array_equal(var.values, var_dict["values"])  # type: ignore[arg-type]
    assert np.array_equal(var.variances, var_dict["variances"])  # type: ignore[arg-type]
    assert var.unit == sc.units.one


@pytest.mark.parametrize('dtype', _NORMAL_DTYPES)
@pytest.mark.parametrize(
    'shape',
    ['0', '0,0', '2,0', '0,2', '0,0,0', '2,0,0', '0,2,0', '0,0,2'],
)
def test_empty_variable_from_dict(dtype: str, shape: str) -> None:
    shape_tuple: tuple[int, ...] = tuple(int(s) for s in shape.split(','))
    dims = tuple(f'dim{i}' for i, _ in enumerate(shape_tuple))
    unit: str | None
    time_unit = 's'
    *_, kwargs = _make_empty_variable_kwargs(
        dtype,
        shape_tuple,
        # See note in `_get_dtype_hint()`: datetimes should come with a
        # unit
        unit=time_unit if dtype == 'datetime64' else None,
    )
    empty = kwargs['values']
    unit = {'int64': '1E6 m', 'affine_transform3': '2'}.get(dtype)
    var_dict = {
        "dims": dims,
        "shape": shape_tuple,
        "values": empty,
        "dtype": dtype,
    }
    if unit:
        var_dict['unit'] = unit

    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == var_dict["dims"]
    assert var.shape == shape_tuple
    assert np.array_equal(var.values, empty)
    if unit is None:
        expected_unit = {
            'datetime64': sc.Unit(time_unit),
            'PyObject': None,
            'string': None,
            'bool': None,
        }.get(dtype, sc.Unit('1'))
        assert var.unit == expected_unit
    else:
        assert var.unit == sc.Unit(unit)


@pytest.mark.parametrize('dtype', ['float64', 'float32'])
@pytest.mark.parametrize(
    'shape',
    ['0', '0,0', '2,0', '0,2', '0,0,0', '2,0,0', '0,2,0', '0,0,2'],
)
@pytest.mark.parametrize('has_var', [True, False])
def test_empty_variable_variances_from_dict(
    dtype: str,
    shape: str,
    has_var: bool,
) -> None:
    shape_tuple: tuple[int, ...] = tuple(int(s) for s in shape.split(','))
    empty = np.array([], dtype=dtype).reshape(shape_tuple)
    var_dict = {
        "dims": [f'dim{i}' for i, _ in enumerate(shape_tuple, 1)],
        "shape": shape_tuple,
        "values": empty,
        "dtype": dtype,
        "variances": empty.copy() if has_var else None,
    }
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    variances = var.variances

    if not has_var:
        assert variances is None
        return
    assert variances.shape == shape_tuple
    assert not variances.size


def test_variable_0D_from_dict() -> None:
    var_dict = {"dims": (), "values": 17.0, "variances": 0.2}
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == ()
    assert var.shape == ()
    assert var.value == 17.0
    assert var.variance == 0.2
    assert var.unit == sc.units.one


@pytest.mark.parametrize('ndim', [1, 2])
@pytest.mark.parametrize('dtype', ['string', 'PyObject'])
def test_variable_nonnative_from_dict(
    ndim: Literal[1, 2],
    dtype: Literal['string', 'PyObject'],
) -> None:
    if ndim > 1:
        dims: tuple[str, ...] = ('x', 'y')
        shape: tuple[int, ...] = (2, 5)
    else:
        dims, shape = ('x',), (10,)
    data = np.random.choice(['foo', 'bar', 'baz'], np.prod(shape))
    if dtype == 'PyObject':
        data = data.astype(object)
    var_dict = {"dims": dims, "shape": shape, "values": data, "dtype": dtype}
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == var_dict["dims"] == dims
    assert var.shape == shape
    assert np.array_equal(
        sc.to_dict(var)["values"],
        var_dict["values"],
    )
    assert var.dtype == getattr(sc.DType, dtype)


@pytest.mark.parametrize('dtype', ['string', 'PyObject'])
def test_variable_0D_nonnative_from_dict(
    dtype: Literal['string', 'PyObject'],
) -> None:
    var_dict = {"dims": (), "values": 'foo', "dtype": dtype}
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == var_dict["dims"]
    assert var.shape == ()
    assert np.array_equal(np.array(var.values), np.array('foo'))
    assert var.dtype == getattr(sc.DType, dtype)


@pytest.mark.parametrize('ndim', [1, 2])
def test_variable_vector_from_dict(ndim: Literal[1, 2]) -> None:
    if ndim > 1:
        dims: tuple[str, ...] = ('x', 'y')
        shape: tuple[int, ...] = (2, 2)
        expected: list[Any] = [
            [[0, 1, 2], [3, 4, 5]],
            [[6, 7, 8], [9, 10, 11]],
        ]
    else:
        dims, shape = ('x',), (4,)
        expected = [[0, 1, 2], [3, 4, 5], [6, 7, 8], [9, 10, 11]]
    var_dict = {
        "dims": dims,
        "values": np.arange(12).reshape((*shape, 3)),
        "dtype": "vector3",
    }
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == var_dict["dims"] == dims
    assert var.shape == shape
    assert np.array_equal(np.array(var.values), expected)
    assert var.unit == sc.units.one
    assert var.dtype == sc.DType.vector3


def test_variable_0D_vector_from_dict() -> None:
    var_dict = {"dims": (), "values": [1, 2, 3], "dtype": "vector3"}
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == ()
    assert var.shape == ()
    assert np.array_equal(np.array(var.values), [1, 2, 3])
    assert var.unit == sc.units.one
    assert var.dtype == sc.DType.vector3


@pytest.mark.parametrize('ndim', [1, 2])
@pytest.mark.parametrize('transform', ['linear', 'affine'])
def test_variable_transform_from_dict(
    ndim: Literal[1, 2],
    transform: Literal['linear', 'affine'],
) -> None:
    if ndim > 1:
        dims: tuple[str, ...] = ('x', 'y')
        shape: tuple[int, ...] = (2, 2)
    else:
        dims, shape = ('x',), (4,)

    data = np.arange(36).reshape((*shape, 3, 3))
    if transform == 'affine':
        data, data_ = np.zeros((*shape, 4, 4), dtype=float), data
        data[..., :-1, :-1] = data_
        data[..., -1] = [3, 2, 1, 1]
        dtype = 'affine_transform3'
    else:
        dtype = 'linear_transform3'

    var_dict = {"dims": dims, "values": data, "dtype": dtype}
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == var_dict["dims"]
    assert var.shape == shape
    assert np.array_equal(np.array(var.values), var_dict["values"])  # type: ignore[arg-type]
    assert var.unit == sc.units.one
    assert var.dtype == getattr(sc.DType, dtype)


@pytest.mark.parametrize('transform', ['linear', 'affine'])
def test_variable_0D_transform_from_dict(
    transform: Literal['linear', 'affine'],
) -> None:
    data = np.arange(9).reshape(3, 3)
    if transform == 'affine':
        data, data_ = np.zeros((4, 4), dtype=float), data
        data[:-1, :-1] = data_
        data[-1] = [3, 2, 1, 1]
        dtype = 'affine_transform3'
    else:
        dtype = 'linear_transform3'
    var_dict = {"dims": (), "values": data, "dtype": dtype}
    var = sc.from_dict(var_dict)
    assert isinstance(var, sc.Variable)
    assert var.dims == ()
    assert var.shape == ()
    assert np.array_equal(np.array(var.value), var_dict["values"])  # type: ignore[arg-type]
    assert var.unit == sc.units.one
    assert var.dtype == getattr(sc.DType, dtype)


@pytest.mark.parametrize('ndim', [1, 2])
def test_variable_round_trip(ndim: Literal[1, 2]) -> None:
    var = sc.Variable(
        dims=["x"],
        values=np.arange(10.0),
        variances=np.random.random(10),
        unit=sc.units.m,
    )
    if ndim > 1:
        var = var.fold('x', sizes={'x': 2, 'y': 5})
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


@pytest.mark.parametrize('dtype', _NORMAL_DTYPES)
@pytest.mark.parametrize(
    'shape',
    ['0', '0,0', '2,0', '0,2', '0,0,0', '2,0,0', '0,2,0', '0,0,2'],
)
def test_empty_variable_round_trip(dtype: str, shape: str) -> None:
    shape_tuple: tuple[int, ...] = tuple(int(s) for s in shape.split(','))
    unit: str | None = {
        'vector3': 'm*s**-1',
        'datetime64': 'D',
    }.get(dtype)
    var = _make_empty_variable(dtype, shape_tuple, unit)
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


@pytest.mark.parametrize('dtype', ['float64', 'float32'])
@pytest.mark.parametrize(
    'shape',
    ['0', '0,0', '2,0', '0,2', '0,0,0', '2,0,0', '0,2,0', '0,0,2'],
)
@pytest.mark.parametrize('has_var', [True, False])
def test_empty_variable_variances_round_trip(
    dtype: str,
    shape: str,
    has_var: bool,
) -> None:
    shape_tuple: tuple[int, ...] = tuple(int(s) for s in shape.split(','))
    var = _make_empty_variable(dtype, (0,))
    if has_var:
        var.variances = []
    var_1 = var.fold(
        var.dims[0],
        dims=[f'dim{i}' for i, _ in enumerate(shape_tuple, 1)],
        shape=shape_tuple,
    )

    variances_1 = var_1.variances
    var_2 = sc.from_dict(sc.to_dict(var_1))
    assert isinstance(var_2, sc.Variable)
    variances_2 = var_2.variances
    if has_var:
        assert variances_1.shape == shape_tuple
        assert not variances_1.size
        assert np.array_equal(variances_1, variances_2)
    else:
        assert variances_1 is None is variances_2


def test_variable_0D_round_trip() -> None:
    var = 12.0 * sc.units.one
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


@pytest.mark.parametrize('ndim', [1, 2])
@pytest.mark.parametrize('dtype', ['string', 'PyObject'])
def test_variable_nonnative_round_trip(
    ndim: Literal[1, 2],
    dtype: Literal['string', 'PyObject'],
) -> None:
    data = np.random.choice(['foo', 'bar', 'baz'], 10)
    if dtype == 'PyObject':
        data = data.astype(object)
    var = sc.Variable(dims=('x',), values=data)
    if ndim > 1:
        var = var.fold('x', sizes={'x': 2, 'y': 5})
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


@pytest.mark.parametrize('dtype', ['string', 'PyObject'])
def test_variable_0D_nonnative_round_trip(
    dtype: Literal['string', 'PyObject'],
) -> None:
    var = sc.Variable(
        values='foo',
        dims=(),
        dtype=str if dtype == 'string' else object,
    )
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


@pytest.mark.parametrize('ndim', [1, 2])
def test_variable_vector_round_trip(ndim: Literal[1, 2]) -> None:
    var = sc.vectors(dims=['x'], values=np.random.random([10, 3]))
    if ndim > 1:
        var = var.fold('x', sizes={'x': 2, 'y': 5})
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


def test_variable_0D_vector_round_trip() -> None:
    var = sc.vector(value=[1, 2, 3])
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


@pytest.mark.parametrize('ndim', [1, 2])
@pytest.mark.parametrize('transform', ['linear', 'affine'])
def test_variable_transform_round_trip(
    ndim: Literal[1, 2],
    transform: Literal['linear', 'affine'],
) -> None:
    data = np.array(
        [
            np.arange(9.0).reshape(3, 3),
            np.arange(5.0, 14.0).reshape(3, 3),
            np.arange(1.0, 10.0).reshape(3, 3),
            np.arange(2.0, 11.0).reshape(3, 3),
        ]
    )
    var = sc.spatial.linear_transforms(dims=['x'], values=data, unit=sc.units.m)
    if ndim > 1:
        var = var.fold('x', sizes={'x': 2, 'y': 2})
    if transform == 'affine':
        var = var * sc.spatial.translation(value=np.array([3, 2, 1]), unit='m')
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


def test_variable_0D_matrix_round_trip() -> None:
    var = sc.spatial.linear_transform(value=np.arange(1, 10).reshape(3, 3))
    assert sc.identical(var, sc.from_dict(sc.to_dict(var)))


def test_data_array_to_dict() -> None:
    da = sc.DataArray(
        coords={
            "x": sc.Variable(dims=["x"], values=np.arange(10.0)),
            "y": sc.Variable(dims=["y"], values=np.arange(5), unit=sc.units.m),
        },
        masks={
            "amask": sc.Variable(dims=["y"], values=[True, True, False, True, False])
        },
        data=sc.Variable(dims=["y", "x"], values=np.random.random([5, 10])),
    )
    da.coords.set_aligned("y", False)
    da_dict = sc.to_dict(da)
    assert da_dict["data"]["dims"] == da.dims
    assert da_dict["data"]["shape"] == da.shape
    assert np.array_equal(da_dict["data"]["values"], da.values)
    assert np.array_equal(da_dict["data"]["variances"], da.variances)
    assert da_dict["data"]["unit"] == da.unit
    assert da_dict["data"]["dtype"] == da.dtype
    assert sc.identical(sc.from_dict(da_dict["coords"]["x"]), da.coords["x"])
    assert sc.identical(sc.from_dict(da_dict["coords"]["y"]), da.coords["y"])
    assert not da_dict["coords"]["y"]["aligned"]
    assert sc.identical(sc.from_dict(da_dict["masks"]["amask"]), da.masks["amask"])


def test_data_array_from_dict() -> None:
    da_dict: dict[str, Any] = {
        "coords": {
            "x": {"dims": ["x"], "values": np.arange(10), "aligned": False},
            "y": {"dims": ["y"], "values": np.arange(5), "unit": sc.units.m},
        },
        "masks": {"amask": {"dims": ["y"], "values": [True, True, False, True, False]}},
        "data": {"dims": ["y", "x"], "values": np.random.random([5, 10])},
    }
    da = sc.from_dict(da_dict)
    assert isinstance(da, sc.DataArray)
    assert sc.identical(da.coords["x"], sc.from_dict(da_dict["coords"]["x"]))
    assert not da.coords["x"].aligned
    assert sc.identical(da.coords["y"], sc.from_dict(da_dict["coords"]["y"]))
    assert da.coords["y"].aligned
    assert sc.identical(da.masks["amask"], sc.from_dict(da_dict["masks"]["amask"]))
    assert sc.identical(da.data, sc.from_dict(da_dict["data"]))


def test_data_array_round_trip() -> None:
    da = sc.DataArray(
        coords={
            "x": sc.Variable(dims=["x"], values=np.arange(10.0)),
            "y": sc.Variable(dims=["y"], values=np.arange(5), unit=sc.units.m),
        },
        masks={
            "amask": sc.Variable(dims=["y"], values=[True, True, False, True, False])
        },
        data=sc.Variable(dims=["y", "x"], values=np.random.random([5, 10])),
    )
    assert sc.identical(da, sc.from_dict(sc.to_dict(da)))


def test_dataset_to_dict() -> None:
    ds = sc.Dataset(
        {
            'a': sc.array(dims=["y", "x"], values=np.random.random([5, 10])),
            "b": sc.array(
                dims=["y", "x"],
                values=np.random.random([5, 10]),
                variances=np.random.random([5, 10]),
                unit=sc.units.s,
            ),
        },
        coords={
            "x": sc.array(dims=["x"], values=np.arange(10)),
            "y": sc.array(dims=["y"], values=np.arange(5), unit=sc.units.m),
        },
    )
    ds["a"].masks["amask"] = sc.array(
        dims=["y"], values=[True, True, False, True, False]
    )
    # Note that attributes complicate things here, as they are duplicated in
    # each entry during the conversion to dict. So for now, we leave attributes
    # out.
    ds_dict = sc.to_dict(ds)
    assert sc.identical(sc.from_dict(ds_dict["a"]), ds["a"])
    assert sc.identical(sc.from_dict(ds_dict["b"]), ds["b"])


def test_dataset_from_dict() -> None:
    ds_dict = {
        "a": {
            "coords": {
                "x": {"dims": ["x"], "values": np.arange(10)},
                "y": {"dims": ["y"], "values": np.arange(5), "unit": sc.units.m},
            },
            "masks": {
                "amask": {"dims": ["y"], "values": [True, True, False, True, False]}
            },
            "data": {"dims": ["y", "x"], "values": np.random.random([5, 10])},
        },
        "b": {
            "coords": {
                "x": {"dims": ["x"], "values": np.arange(10)},
                "y": {"dims": ["y"], "values": np.arange(5), "unit": sc.units.m},
            },
            "masks": {
                "amask": {"dims": ["y"], "values": [True, True, False, True, False]}
            },
            "data": {
                "dims": ["y", "x"],
                "values": np.random.random([5, 10]),
                "variances": np.random.random([5, 10]),
            },
        },
    }
    ds = sc.from_dict(ds_dict)
    assert isinstance(ds, sc.Dataset)
    assert sc.identical(ds["a"], sc.from_dict(ds_dict["a"]))
    assert sc.identical(ds["b"], sc.from_dict(ds_dict["b"]))


def test_dataset_round_trip() -> None:
    ds = sc.Dataset(
        {
            "a": sc.array(dims=["y", "x"], values=np.random.random([5, 10])),
            "b": sc.array(
                dims=["y", "x"],
                values=np.random.random([5, 10]),
                variances=np.random.random([5, 10]),
                unit=sc.units.s,
            ),
        },
        coords={
            "x": sc.Variable(dims=["x"], values=np.arange(10)),
            "y": sc.Variable(dims=["y"], values=np.arange(5), unit=sc.units.m),
        },
    )
    ds["a"].masks["amask"] = sc.Variable(
        dims=["y"], values=[True, True, False, True, False]
    )
    # Note that round trip would not work if attrs are present, since they get
    # become a per-item attribute during the conversion to dict.
    assert sc.identical(ds, sc.from_dict(sc.to_dict(ds)))
