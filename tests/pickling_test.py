# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Terence Tsang

# ruff: noqa: S301

import multiprocessing
import pickle
import re
import sys
from collections.abc import Callable, Sequence
from datetime import datetime
from functools import partial
from operator import eq
from pathlib import Path
from typing import Any, cast

import numpy as np
import pytest
from scipy.spatial.transform import Rotation

import scipp as sc
from scipp import spatial

_64_BIT = sys.maxsize > (2 << 32)
_DEFAULT_INT_TYPE = str(np.dtype(int))
_DEFAULT_FLOAT_TYPE = str(np.dtype(float))


def _roundtrip[T](obj: T, cmp: Callable[[T, T], bool] = eq) -> None:
    """Test round-tripping in-process ``obj`` via :mod:`pickle`."""
    serialized = pickle.dumps(obj)
    deserialized = pickle.loads(serialized)
    assert deserialized is not obj
    assert cmp(obj, deserialized)


def _cmp_units(u1: sc.Unit, u2: sc.Unit) -> bool:
    if u1 == u2:
        return True
    # Assure the typechecker that the mults are numbers or it will
    # complain about the `isnan()` calls
    mult1 = cast(float, (d1 := u1.to_dict()).pop('multiplier', 1))
    mult2 = cast(float, (d2 := u2.to_dict()).pop('multiplier', 1))
    if d1 != d2:
        return False
    # Coerce to Python bools to appease the typechecker
    return bool(np.isnan(mult1) and np.isnan(mult2))


def _cmp_arrays(a1: sc.DataArray, a2: sc.DataArray) -> bool:
    """
    :func:`scipp.identical()` doesn't check the array names, so do it
    here.

    Example
    -------
    >>> from operator import is_
    >>> from scipp import DataArray, array, identical

    >>> a1 = DataArray(data=array(values=[5], dims=['_']), name='foo')
    >>> a2 = a1.copy()
    >>> identical(a1, a2), is_(a1, a2), _cmp_arrays(a1, a2)
    (True, False, True)

    >>> a2.name = 'bar'
    >>> a2.name
    'bar'
    >>> identical(a1, a2), is_(a1, a2), _cmp_arrays(a1, a2)
    (True, False, False)
    """
    return a1.name == a2.name and sc.identical(a1, a2)


def _cmp_datasets(ds1: sc.Dataset, ds2: sc.Dataset) -> bool:
    if (keys := set(ds1)) != set(ds2):
        return False
    return all(_cmp_arrays(ds1[key], ds2[key]) for key in keys)


def _cmp_groups(g1: sc.DataGroup[Any], g2: sc.DataGroup[Any]) -> bool:
    if (keys := set(g1)) != set(g2):
        return False
    for key in keys:
        value_1 = g1[key]
        value_2 = g2[key]
        for Class, cmp in [
            (sc.DataGroup, _cmp_groups),
            (sc.Dataset, _cmp_datasets),
            (sc.DataArray, _cmp_arrays),
            (sc.Variable, sc.identical),
        ]:
            if not (isinstance(value_1, Class) and isinstance(value_2, Class)):
                continue
            if cast(Callable[[Any, Any], bool], cmp)(value_1, value_2):
                break
            else:
                return False
        else:
            if not sc.identical(value_1, value_2):
                return False
    return True


def _get_dims_and_shape(
    sizes: dict[str, int],
) -> tuple[tuple[str, ...], tuple[int, ...]]:
    if not sizes:
        return (), ()
    dims, shape = zip(*sizes.items(), strict=True)
    return dims, shape


def _get_rng(rng: np.random.Generator | Any = 42) -> np.random.Generator:
    if isinstance(rng, np.random.Generator):
        return rng
    return np.random.default_rng(rng)


def _sizes_dict(ndim: int, size: int = 10) -> dict[str, int]:
    return dict.fromkeys([f'dim{i}' for i in range(1, ndim + 1)], size)


def _named_parametrizations[ArgOrArgs: (Sequence[Any], Any)](
    params: str | Sequence[str],
    /,
    *case_dicts: dict[str, ArgOrArgs],
    **more_cases: ArgOrArgs,
) -> Callable[[Callable[..., None]], Callable[..., None]]:
    """Convenience wrapper around :func:`pytest.mark.parametrize` and
    :func:`pytest.param` for constructing explicitly-named
    parametrization cases.
    """
    if isinstance(params, str):
        param_names: list[str] = [p.strip() for p in params.split(',')]
        args_unpacked = len(param_names) == 1
    else:
        param_names = list(params)
        args_unpacked = False

    all_cases: list[dict[str, ArgOrArgs]] = list(case_dicts)
    all_cases.append(more_cases)

    param_sets = [
        pytest.param(*((params,) if args_unpacked else params), id=name)
        for cases in all_cases
        for name, params in cases.items()
    ]
    return pytest.mark.parametrize(params, param_sets)


# --------------------------- `DType` tests ----------------------------


@pytest.mark.parametrize(
    ('expected', 'dtypes'),
    # XXX: will there ever possibly be `DType` objects that don't
    # correspond to one of the class members?
    [
        ('int64', ['int64', np.int64]),
        ('float64', ['float64', np.float64]),
        ('int32', ['int32', np.int32]),
        ('float32', ['float32', np.float32]),
        ('int64' if _64_BIT else 'int32', [int]),
        ('float64' if _64_BIT else 'float32', [float]),
        ('bool', ['bool', np.bool_, bool]),
        ('string', ['string', np.str_, str]),
        ('datetime64', ['datetime64', np.datetime64]),
        ('uint64', ['uint64', np.uint64]),
    ]
    + [
        (dtype, None)
        for dtype in [
            'DataArray',
            'Dataset',
            'PyObject',
            'Variable',
            'DataArrayView',
            'DatasetView',
            'VariableView',
            'affine_transform3',
            'linear_transform3',
            'rotation3',
            'translation3',
            'vector3',
        ]
    ],
)
def test_pickling_dtypes(expected: str, dtypes: list[Any] | None) -> None:
    """
    Test that :class:`DType`s can be round-tripped in-process.
    """
    # Set up the dtype
    if dtypes is None:
        dtypes = [expected]
    for dtype in dtypes:
        try:
            dt = sc.DType(dtype)
        except TypeError:
            assert isinstance(dtype, str)
            dt = getattr(sc.DType, dtype)
        else:
            assert dt == getattr(sc.DType, expected)
        # Actual comparison
        _roundtrip(dt)


# ---------------------------- `Unit` tests ----------------------------


@pytest.mark.parametrize(
    'unit',
    [
        'm',  # Simple unit
        '3.14159 m',  # Unit + multiplier
        'J*m*kg**-1',  # Composite unit
        '6.022E23',  # Dimensionless
        # Edge cases: infinities and nans
        'inf N*m**-2',
        '-inf s',
        'nan m',
    ],
)
def test_pickling_units(unit: str) -> None:
    """
    Test that :class:`Unit`s can be round-tripped in-process.
    """
    _roundtrip(sc.Unit(unit), _cmp_units)


# -------------------------- `Variable` tests --------------------------


def _get_translation() -> sc.Variable:
    return spatial.translation(value=np.array([3.0, 2.0, 1.0]), unit='m')


def _get_rotation() -> sc.Variable:
    return spatial.rotation(
        value=Rotation.from_rotvec([1.0, 2.0, 3.0]).as_quat(),
    )


def _get_numeric_var(
    dtype: sc.DType,
    sizes: dict[str, int],
    *,
    rng: np.random.Generator | Any = 42,
    max_randint: int = 50,
    add_variances: bool = False,
    unit: str | None = None,
) -> sc.Variable:
    rng = _get_rng(rng)
    dims, shape = _get_dims_and_shape(sizes)
    var = sc.empty(dtype=dtype, dims=dims, shape=shape, unit=unit)

    item_shape = {
        'vector3': (3,),
        'translation3': (3,),
        'rotation3': (4,),
        'linear_transform3': (3, 3),
        'affine_transform3': (4, 4),
    }.get(str(dtype), ())
    if str(dtype).startswith(('int', 'uint')):
        assert not add_variances
        var.values = rng.integers(0, max_randint, shape)
    else:
        var.values = rng.random(total_shape := (*shape, *item_shape))
        if add_variances:
            var.variances = rng.random(total_shape)

    assert var.dtype == dtype
    assert var.dims == dims
    assert var.shape == shape
    assert (var.variances is not None) == add_variances

    return var


def _get_non_numeric_var(
    dtype: sc.DType,
    choices: Sequence[Any] | np.ndarray,
    sizes: dict[str, int],
    *,
    rng: np.random.Generator | Any = 42,
    unit: str | None = None,
) -> sc.Variable:
    rng = _get_rng(rng)
    dims, shape = _get_dims_and_shape(sizes)
    if fold := (len(shape) > 1):
        flat_dims: tuple[str, ...] = ('_',)
        flat_shape: tuple[int, ...] = (int(np.prod(shape)),)
    else:
        flat_dims, flat_shape = dims, shape

    var = sc.empty(dtype=dtype, dims=flat_dims, shape=flat_shape, unit=unit)
    setattr(
        var,
        'values' if flat_shape else 'value',
        rng.choice(choices, flat_shape),
    )
    if fold:
        var = var.fold('_', dims=dims, shape=shape)

    assert var.dtype == dtype
    assert var.dims == dims
    assert var.shape == shape

    return var


# Don't listen to `ruff`, timezone gives us nothing but trouble here
_EPOCH = np.datetime64(datetime.fromtimestamp(0.0).isoformat())  # noqa: DTZ006

_VAR_WITH_VARS = _get_numeric_var(
    sc.DType.float64,
    _sizes_dict(2),
    add_variances=True,
)

_VAR_SCALAR_TEST_CASES = {
    'scalar_int': (sc.scalar(5), _DEFAULT_INT_TYPE, ()),
    'scalar_str': (sc.scalar('foo'), 'string', ()),
    'scalar_datetime': (sc.scalar(_EPOCH), 'datetime64', ()),
    'scalar_object': (sc.scalar('foo', dtype=object), 'PyObject', ()),
    'scalar_float_with_variance': (
        sc.scalar(5.0, variance=0.25),
        _DEFAULT_FLOAT_TYPE,
        (),
    ),
    'scalar_vector_with_unit': (sc.vector(range(1, 4), unit='m'), 'vector3', ()),
    'scalar_translation': (_get_translation(), 'translation3', ()),
    'scalar_rotation': (_get_rotation(), 'rotation3', ()),
    'scalar_affine_transformation': (
        _get_translation() * _get_rotation(),
        'affine_transform3',
        (),
    ),
}
_VAR_INT_TEST_CASES = {
    f'ints_{ndim}D': (
        _get_numeric_var(sc.DType(int), _sizes_dict(ndim, (size := 5))),
        _DEFAULT_INT_TYPE,
        (size,) * ndim,
    )
    for ndim in [1, 2, 3]
}
_VAR_INT_FOLDED_TEST_CASES = {
    f'ints_{ndim}D_folded': (
        _get_numeric_var(sc.DType(int), {'_': (size := 5) ** ndim}).fold(
            '_',
            sizes=_sizes_dict(ndim, size),
        ),
        _DEFAULT_INT_TYPE,
        (size,) * ndim,
    )
    for ndim in [1, 2, 3]
}
_VAR_FLOAT_TEST_CASES = {
    f'{dtype}s_{ndim}D': (
        _get_numeric_var(
            getattr(sc.DType, dtype),
            _sizes_dict(ndim, (size := 10)),
        ),
        dtype,
        (size,) * ndim,
    )
    for dtype, ndim in [('float32', 1), ('float64', 1), ('float64', 3)]
}
_VAR_FLOAT_SPECIAL_TEST_CASES = {
    'float64s_2D_with_variables': (_VAR_WITH_VARS, 'float64', (10, 10)),
}
_VAR_LINALG_TEST_CASES = {
    f'{name}_{ndim}D': (
        _get_numeric_var(dtype, _sizes_dict(ndim, (size := 10))),
        str(dtype),
        (size,) * ndim,
    )
    for name, dtype in [
        ('vecs', sc.DType.vector3),
        ('translations', sc.DType.translation3),
        ('rotations', sc.DType.rotation3),
        ('lintr', sc.DType.linear_transform3),
        ('afftr', sc.DType.affine_transform3),
    ]
    for ndim in [1, 2, 3]
}
_VAR_NONNATIVE_TEST_CASES = {
    f'{name}s_{ndim}D': (
        _get_non_numeric_var(
            sc.DType(hint),
            np.array(['foo', 'bar', 'baz']).astype(hint),
            _sizes_dict(ndim, (size := 10)),
        ),
        str(sc.DType(hint)),
        (size,) * ndim,
    )
    for name, hint in [('str', str), ('obj', object)]
    for ndim in [1, 2, 3]
}


@_named_parametrizations(
    ('var', 'dtype', 'shape'),
    **_VAR_SCALAR_TEST_CASES,
    **_VAR_FLOAT_TEST_CASES,
    **_VAR_FLOAT_SPECIAL_TEST_CASES,
    **_VAR_INT_TEST_CASES,
    **_VAR_INT_FOLDED_TEST_CASES,
    **_VAR_LINALG_TEST_CASES,
    **_VAR_NONNATIVE_TEST_CASES,
)
def test_pickling_unbinned_variables(
    var: sc.Variable,
    dtype: str,
    shape: tuple[int, ...],
) -> None:
    """
    With handpicked examples, test that un-binned :class:`Variable`s can
    be round-tripped in-process.
    """
    assert var.dtype == getattr(sc.DType, dtype)
    assert var.shape == shape

    _roundtrip(var, sc.identical)


@pytest.mark.parametrize(
    'shape',
    [
        '0',
        '0,1',
        '0,2',
        '1,0',
        '2,0',
        '0,0,1',
        '0,0,2',
        '0,1,0',
        '0,2,0',
        '1,0,0',
        '2,0,0',
    ],
)
def test_pickling_empty_variables(shape: str) -> None:
    """
    Test that empty :class:`Variable`s (i.e. those with one or more
    dimensions being zero-sized) can be round-tripped in-process.

    See also
    --------
    tests/compat/dict_test.py::test_empty_variable_to_dict()
    """
    shape_tuple = tuple(int(i) for i in shape.split(','))
    dims = tuple(f'dim{i}' for i, _ in enumerate(shape_tuple))
    var = sc.empty(shape=(0,), dims=('_',))
    var = var.fold('_', shape=shape_tuple, dims=dims)
    assert var.shape == shape_tuple

    _roundtrip(var, sc.identical)


# ------------------------- `DataArray` tests --------------------------


def _get_numeric_array(
    dtype: sc.DType,
    sizes: dict[str, int],
    *,
    rng: np.random.Generator | Any = 42,
    add_coords: bool = False,
    add_mask: bool = False,
    unit: str | None = None,
    name: str | None = None,
) -> sc.DataArray:
    rng = _get_rng(rng)
    var = _get_numeric_var(dtype, sizes, rng=rng, unit=unit)
    return _massage_data_array(var, rng, add_coords, add_mask, name)


def _get_non_numeric_array(
    dtype: sc.DType,
    choices: Sequence[Any] | np.ndarray,
    sizes: dict[str, int],
    *,
    rng: np.random.Generator | Any = 42,
    add_coords: bool = False,
    add_mask: bool = False,
    unit: str | None = None,
    name: str | None = None,
) -> sc.DataArray:
    rng = _get_rng(rng)
    var = _get_non_numeric_var(dtype, choices, sizes, rng=rng, unit=unit)
    return _massage_data_array(var, rng, add_coords, add_mask, name)


def _massage_data_array(
    var: sc.Variable,
    rng: np.random.Generator,
    add_coords: bool = False,
    add_mask: bool = False,
    name: str | None = None,
) -> sc.DataArray:
    array = sc.DataArray(data=var)

    if name:
        array.name = name
    if add_coords:
        array = array.assign_coords(
            {
                dim: sc.linspace(start=0, stop=1, num=size, dim=dim)
                for dim, size in var.sizes.items()
            }
        )
    if add_mask:
        array.masks['mask'] = sc.array(
            values=rng.choice([True, False], var.shape).astype(bool),
            dims=var.dims,
        )

    if name:
        assert array.name == name
    else:
        assert not array.name
    assert bool(array.coords) == add_coords
    assert bool(array.masks) == add_mask
    return array


@_named_parametrizations(
    'da',
    scalar_float=_get_numeric_array(sc.DType(float), {}),
    scalar_obj=_get_non_numeric_array(sc.DType(object), [None], {}),
    nothing_2D=_get_non_numeric_array(sc.DType.string, [''], _sizes_dict(2)),
    int_1D=_get_numeric_array(sc.DType(int), _sizes_dict(1)),
    float_1D_with_name=(
        _get_numeric_array(sc.DType(float), _sizes_dict(1), name='foo')
    ),
    float_2D_with_mask=(
        _get_numeric_array(sc.DType(float), _sizes_dict(2), name='foo')
    ),
    vec_3D=_get_numeric_array(sc.DType.vector3, _sizes_dict(3)),
    linear_transform_1D=(
        _get_numeric_array(sc.DType.linear_transform3, _sizes_dict(1))
    ),
    string_2D=_get_non_numeric_array(
        sc.DType.string,
        ['foo', 'bar', 'baz'],
        _sizes_dict(2),
    ),
    obj_2D=_get_non_numeric_array(
        sc.DType.PyObject,
        [None, Ellipsis, NotImplemented],
        _sizes_dict(2),
    ),
)
def test_pickling_unbinned_data_arrays(da: sc.DataArray) -> None:
    """
    With handpicked examples, test that un-binned :class:`DataArray`s
    can be round-tripped in-process.
    """
    _roundtrip(da, _cmp_arrays)


# -------------------------- `Dataset` tests ---------------------------


def _get_dataset_implcit_coords(  # `.coords` extracted from arrays
    foo: sc.Variable, bar: sc.Variable, x: sc.Variable, t: sc.Variable, **_: Any
) -> sc.Dataset:
    foo_da = sc.DataArray(data=foo, coords={'t': t})
    bar_da = sc.DataArray(data=bar, coords={'x': x})
    ds = sc.Dataset({'foo': foo_da, 'bar': bar_da})
    assert set(ds.coords) == {'x', 't'}
    assert set(ds) == {'foo', 'bar'}
    return ds


def _get_dataset_explcit_coords(  # `.coords` imposed on variables
    foo: sc.Variable, bar: sc.Variable, x: sc.Variable, t: sc.Variable, **_: Any
) -> sc.Dataset:
    ds = sc.Dataset({'foo': foo, 'bar': bar}, coords={'x': x, 't': t})
    assert set(ds.coords) == {'x', 't'}
    assert set(ds) == {'foo', 'bar'}
    return ds


def _get_empty_dataset(
    x: sc.Variable, t: sc.Variable, attach_coords: bool = False, **_: Any
) -> sc.Dataset:
    if attach_coords:
        ds = sc.Dataset({}, coords={'x': x, 't': t})
        assert set(ds.coords) == {'x', 't'}
    else:
        ds = sc.Dataset({})
        assert not ds.coords
    assert not len(ds)
    return ds


def _get_dataset_with_masked_entry(
    foo: sc.Variable, bar: sc.Variable, x: sc.Variable, t: sc.Variable, **_: Any
) -> sc.Dataset:
    foo_da = sc.DataArray(
        data=foo,
        coords={'t': t},
        masks={'t': t < sc.scalar(value=5.0, unit='s')},
    )
    ds = sc.Dataset(  # Mixed coords source
        cast(dict[str, Any], {'foo': foo_da, 'bar': bar}),
        coords={'x': x},
    )
    assert set(ds.coords) == {'x', 't'}
    assert set(ds) == {'foo', 'bar'}
    mask = ds['foo'].masks['t']
    assert (mask['t', 0].value, mask['t', -1].value) == (True, False)
    assert not ds['bar'].masks
    return ds


def _get_dataset_with_renamed_entry(
    foo: sc.Variable, bar: sc.Variable, x: sc.Variable, t: sc.Variable, **_: Any
) -> sc.Dataset:
    foo_da = sc.DataArray(
        data=foo,
        coords={'t': t},
        name='baz',  # Overwritten when accessed from the dataset
    )
    ds = sc.Dataset(  # Mixed coords source
        cast(dict[str, Any], {'foo': foo_da, 'bar': bar}),
        coords={'x': x},
    )
    assert set(ds.coords) == {'x', 't'}
    assert set(ds) == {'foo', 'bar'}
    assert ds['foo'].name == 'foo'
    assert ds['bar'].name == 'bar'
    return ds


@_named_parametrizations(
    'get_dataset',
    implicit_coords_and_arrays=_get_dataset_implcit_coords,
    explicit_coords_and_variables=_get_dataset_explcit_coords,
    masked_entries=_get_dataset_with_masked_entry,
    renamed_entries=_get_dataset_with_renamed_entry,
    empty_without_coords=_get_empty_dataset,
    empty_with_coords=partial(_get_empty_dataset, attach_coords=True),
)
def test_pickling_unbinned_datasets(
    get_dataset: Callable[..., sc.Dataset],
) -> None:
    """
    With handpicked examples, test that un-binned :class:`Dataset`s
    can be round-tripped in-process.
    """
    rng = _get_rng()
    n = 100

    t = sc.linspace(start=0, stop=10, num=n, dim='t', unit='s')
    x = t * sc.Unit('50 m*s**-1')
    foo = sc.array(values=rng.random(n), dims=['t'])
    bar = sc.array(values=rng.random(n), dims=['t'])

    ds = get_dataset(t=t, x=x, foo=foo, bar=bar)
    _roundtrip(ds, _cmp_datasets)


# ------------------------- `DataGroup` tests --------------------------


def _get_data_group(
    rng: Any = 42,
    n: int = 100,
    homogeneous: bool = True,
) -> sc.DataGroup[Any]:
    rng = _get_rng(rng)

    t = sc.linspace(start=0, stop=1, num=n, dim='t')
    v = sc.vectors(values=rng.random((n, 3)), dims=['t'])
    y = sc.array(values=rng.random(n), unit='MeV', dims=['t'])

    group = sc.DataGroup(
        v=sc.DataArray(data=v, coords={'t': t}),
        y=sc.Dataset({'y': y}, coords={'t': t}),
    )
    if homogeneous:
        assert len(group) == 2
        assert group.dims == ('t',)
    else:
        group['z'] = sc.array(values=[True], dims=['z'])
        assert len(group) == 3
        assert group.dims == ('t', 'z')
    return group


@_named_parametrizations(
    'group',
    empty=sc.DataGroup[Any](),
    homogeneous=_get_data_group(),
    inhomogeneous=_get_data_group(homogeneous=False),
)
def test_pickling_data_groups(group: sc.DataGroup[Any]) -> None:
    """
    With handpicked examples, test that :class:`DataGroup`s can be
    round-tripped in-process.
    """
    _roundtrip(group, _cmp_groups)


# ------------------------- Binned-data tests --------------------------


def _get_binned_data(
    rng: Any = 42,
    n: int = 100,
    nbins: int = 10,
) -> sc.DataArray:
    times = sc.array(values=_get_rng(rng).random(n), dims=['t'], unit='ms')
    events = sc.DataArray(
        data=sc.ones(shape=[n], dims=['t']),
        coords={'t': times},
    )
    return events.bin(t=nbins)


@_named_parametrizations(
    ('cls', 'pickled', 'cmp'),
    Variable=(sc.Variable, _get_binned_data().data, sc.identical),
    DataArray=(sc.DataArray, _get_binned_data(), _cmp_arrays),
    Dataset=(sc.Dataset, sc.Dataset({'y': _get_binned_data()}), _cmp_datasets),
)
def test_pickling_binned_data[T: (sc.Dataset, sc.DataArray, sc.Variable)](
    cls: type[T],
    pickled: T,
    cmp: Callable[[T, T], bool],
) -> None:
    """
    With handpicked examples, test that binned data can be round-tripped
    in-process.
    """
    assert isinstance(pickled, cls)
    assert pickled.is_binned

    _roundtrip(pickled, cmp)


@_named_parametrizations(
    ('slice', 'should_reduce'),
    full=(slice(None), False),
    eighty_pc=(slice(None, -2), False),
    twenty_pc=(slice(5, 7), True),
)
def test_pickling_binned_and_sliced_var(
    slice: slice,
    should_reduce: bool,
) -> None:
    """
    Test that pickling a binned and sliced variable results in reduction
    and reindexing of the underlying array where appropriate.
    """
    binned = _get_binned_data()
    sliced = binned['t', slice]
    round_tripped = pickle.loads(pickle.dumps(sliced))

    assert isinstance(round_tripped, sc.DataArray)
    assert _cmp_arrays(sliced, round_tripped)
    assert sliced.is_binned
    assert round_tripped.is_binned

    size_before = sliced.bins.constituents['data'].__sizeof__()
    size_after = round_tripped.bins.constituents['data'].__sizeof__()
    assert (size_before == size_after) != should_reduce


# ---------------------------- Misc. tests -----------------------------


def test_multiproc_worker_communication() -> None:
    """
    Integration test: :mod:`scipp` types can be sent to and from
    spawned :mod:`multiprocessing` worker processes.
    """
    base = sc.Dataset(
        {
            'f': sc.array(values=_get_rng(42).random(100), dims=['x']),
            'v': sc.array(
                values=_get_rng(42).random((100, 3)),
                dims=['x'],
                dtype=sc.DType.vector3,
            ),
        },
        coords={
            'x': sc.linspace(start=0, stop=1, num=100, dim='x'),
        },
    )
    with multiprocessing.get_context('spawn').Pool(4) as pool:
        n = 4
        workloads = [base['x', start::n] for start in range(n)]
        results = pool.map(sc.sum, workloads)
    for workload, result in zip(workloads, results, strict=True):
        assert _cmp_datasets(workload.sum(), result)


def test_unpickling_wrong_scipp_version(tmp_path: Path) -> None:
    """
    Test that unpickling a pickled :mod:`scipp` object from the wrong
    version results in an error.
    """
    version_at_pickling = sc.__version__
    version_at_unpickling = sc.__version__ + '.patch'

    unit = sc.Unit('5 m')
    fname = tmp_path / 'unit.pkl'
    with fname.open(mode='wb') as fobj:
        pickle.dump(unit, fobj)

    with pytest.MonkeyPatch.context() as mp:
        mp.setattr('scipp._pickling._VERSION', version_at_unpickling)
        msg = 'version mismatch: .*' + re.escape(
            f'package version = {version_at_unpickling!r}, '
            f'pickled version = {version_at_pickling!r}; '
        )
        with pytest.raises(RuntimeError, match=msg):
            with fname.open(mode='rb') as fobj:
                pickle.load(fobj)
