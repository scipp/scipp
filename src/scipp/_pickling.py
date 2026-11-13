# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Terence Tsang

"""Basic implementation for (un-)pickling `scipp` objects.

This is for the benefit of e.g. interoperability with `multiprocessing`.

Caveats
-------
- This is NOT meant for persisting data; users are directed to either
  `scippnexus` or the HDF5-based scheme (`scipp.serialization`) for
  that purpose.
- There is no special provision for subclasses.
- The pickle serialization should be considered opaque; the schemas'
  consistency should NOT be relied upon, esp. between `scipp` versions.
"""

from __future__ import annotations

from collections.abc import Callable
from copyreg import pickle
from functools import partial
from typing import Any, Literal, NewType, TypedDict, cast

import numpy as np
from numpy.typing import NDArray

from . import __version__ as _version
from .core import (
    Bins,  # noqa: F401
    DataArray,
    Dataset,
    DType,
    Unit,
    Variable,
    bins,
    empty,
)

_Version = NewType('_Version', str)
_SCIPP_VERSION = _Version(_version)
_FOLDED_DTYPES = (
    # These cannot be natively N-D, and are stored as 1-D arrays and
    # `.fold()`-ed into shape. The exception is when the `Variable` was
    # constructed with a quasi-1D array of shape e.g. `(10, 1, 1)`; this
    # permitted by the `Variable` constructor, but indexing for such
    # cases is botched anyway (issue #4018).
    DType.DataArray,
    DType.DataArrayView,
    DType.Dataset,
    DType.DatasetView,
    DType.PyObject,
    DType.Variable,
    DType.VariableView,
)
_LINALG_TYPES = (  # These are always represented as float arrays.
    DType.affine_transform3,
    DType.linear_transform3,
    DType.rotation3,
    DType.translation3,
    DType.vector3,
)

# --------------------------- Infrastructure ---------------------------


class _VarDict[T: Any](TypedDict):
    dtype: DType
    dims: tuple[str, ...]
    shape: tuple[int, ...]
    values: NDArray[T] | T
    variances: NDArray[T] | T | None
    unit: Unit | None
    aligned: bool
    folding_required: bool


class _BinsDict(TypedDict):
    data: Variable | DataArray
    dim: str
    begin: Variable
    end: Variable


class _ArrayDict[T: Any](TypedDict):
    data: Variable
    coords: dict[str, Variable]
    masks: dict[str, Variable]
    name: str


class _DatasetDict(TypedDict):
    # We should be able to just store each `DataArray`'s `.data` and
    # `.masks` (if any) -- the `.name` is always the same as the
    # `Dataset` key and the `.coords` as `Dataset.coords`.
    # TODO: check if this is safe with binned data.
    data: dict[
        str,
        tuple[
            Variable,  # `.data`
            dict[str, Variable],  # `.masks`
        ],
    ]
    coords: dict[str, Variable]


_UnitSerialization = NewType(
    '_UnitSerialization',
    tuple[_Version, dict[str, Any]],
)
_DTypeSerialization = NewType('_DTypeSerialization', tuple[_Version, str])
_VariableSerialization = NewType(
    '_VariableSerialization',
    tuple[
        _Version,
        tuple[Literal[False], _VarDict[Any], dict[str, Any]]
        | tuple[Literal[True], _BinsDict, dict[str, Any]],
    ],
)
_DataArraySerialization = NewType(
    '_DataArraySerialization',
    tuple[_Version, _ArrayDict[Any]],
)
_DatasetSerialization = NewType(
    '_DatasetSerialization',
    tuple[_Version, _DatasetDict],
)


def _check_and_strip_version[T](versioned_tuple: tuple[_Version, T]) -> T:
    version, result = versioned_tuple
    if version == _SCIPP_VERSION:
        return result
    raise RuntimeError(
        'version mismatch: '
        f'`scipp` package version = {_SCIPP_VERSION}, '
        f'pickled version = {version}; '
        'note that pickling is only meant for processes '
        '(e.g. `multiprocessing` workers) using the same Python and '
        'the same `scipp`, and users should use e.g. HDF5 (`scipp.serialize`)'
        'for actual data storage.'
    )


def _get_reconstructor_and_args[T, A](
    pickler: Callable[[T], A],
    unpickler: Callable[[A], T],
    obj: T,
) -> tuple[Callable[[A], T], tuple[A]]:
    return unpickler, (pickler(obj),)


def _register[T, A](
    cls: type[T],
    pickler: Callable[[T], A],
    unpickler: Callable[[A], T],
) -> None:
    pickle(cls, partial(_get_reconstructor_and_args, pickler, unpickler))


# ------------------------ `Variable` handling -------------------------


def _normalize_var_values(var: Variable, field: str = 'values') -> Any:
    """Return the normalized :attr:`Variable.values`.

    This is mainly for dealing with:

    - Linear-algebra types (affine transforms etc.),

    - String/object arrays, and

    - :mod:`numpy` strings.

    Caveats, particularly for the second:

    - The ``.values`` of string and object types are
      ``ElementArrayView``s, which cannot reliably be iterated over,
      despite its having a :meth:`.__len__` and a :meth:`.__iter__`, the
      latter of which doesn't seem to work.

    - They are also often backed by 1-D arrays even when
      multi-dimensional, because the constructor doesn't take
      multi-D arrays (save for the edge case of ), and must be formed
      by :meth:`Variable.fold`-ing a flat instance.

    - The one exception to the multi-dimensional rule above is when the
      trailing dimensions all have size 1, for some reason.
    """
    data = getattr(var, field)

    # Linalg types are always array(-like)s.
    if var.dtype in _LINALG_TYPES:
        return np.array(data)

    # Just the value, nothing to look at here.
    if not var.shape:
        return data

    # Object and string arrays, as well as other entities of nested
    # `scipp` types (e.g. `DType.Variable`), are converted into
    # `ElementArrayView`s by the `Variable` constructor; reverse that.
    # (Note: these should always be flat, save for problematic corner
    # cases which aren't handled consistently anyway (see comment for
    # `_FOLDED_DTYPES`).)
    if var.dtype == DType.string:
        return np.array(data)

    if var.dtype in _FOLDED_DTYPES:
        # `.__iter__()` over object array views
        # (`ElementArrayView_PyObject`) is botched, despite
        # `.__len__()`'s somehow working. This is maybe because of how
        # empty object `Variable`s contain NULL references, which cannot
        # be cast back into Python entities.
        n = len(data)
        array = np.empty(n, dtype=object)
        if var.dtype == DType.PyObject:
            for i in range(n):
                array[i] = data[i]
        else:  # Non-`PyObject` views can be iterated over directly
            array[:] = data
        return array

    return data


def _reduce_bins_constituents(
    binned: Variable,
    reduction_threshold: float = 1.5,
) -> _BinsDict:
    # Sliced binned data contains a copy of the original bins, which
    # is re-indexed and reduced on `.copy()`; see a similar heuristic in
    # `scipp.io.hdf5` to avoid hauling big unused data sections around
    bins_dict = binned.bins.constituents
    underlying_size = bins_dict['data'].sizes[bins_dict['dim']]
    used_size = binned.bins.size().sum().value
    if underlying_size > reduction_threshold * used_size:
        bins_dict = binned.copy().bins.constituents
    return _BinsDict(**bins_dict)  # type: ignore[typeddict-item]


def _var_dict_to_var(v: _VarDict[Any]) -> Variable:
    if fold := v['folding_required']:
        values = cast(np.ndarray, v['values']).flatten()
        shape: tuple[int, ...] = (int(np.prod(v['shape'])),)
        dims: tuple[str, ...] = ('_',)
        if (variances := v['variances']) is not None:
            variances = cast(np.ndarray, variances).flatten()
    else:
        values = v['values']
        shape = v['shape']
        dims = v['dims']
        variances = v['variances']

    var = empty(
        dtype=v['dtype'],
        dims=dims,
        shape=shape,
        unit=v['unit'],
        aligned=v['aligned'],
        with_variances=variances is not None,
    )

    setattr(var, 'values' if shape else 'value', values)
    if variances is not None:
        setattr(var, 'variances' if shape else 'variance', variances)

    if fold:
        return var.fold('_', shape=v['shape'], dims=v['dims'])

    return var


# ------------------------- Pickling functions -------------------------


def _pickle_unit(unit: Unit) -> _UnitSerialization:
    return _UnitSerialization((_SCIPP_VERSION, unit.to_dict()))


def _unpickle_unit(unit: _UnitSerialization) -> Unit:
    unit_dict = _check_and_strip_version(unit)
    return Unit.from_dict(unit_dict)


def _pickle_dtype(dtype: DType) -> _DTypeSerialization:
    return _DTypeSerialization((_SCIPP_VERSION, str(dtype)))


def _unpickle_dtype(dtype: _DTypeSerialization) -> DType:
    dtype_name = _check_and_strip_version(dtype)
    return cast(DType, getattr(DType, dtype_name))


def _pickle_variable(var: Variable) -> _VariableSerialization:
    inst_dict = dict(var.__dict__)

    if var.is_binned:
        bins_dict = _reduce_bins_constituents(var)
        return _VariableSerialization(
            (_SCIPP_VERSION, (True, bins_dict, inst_dict)),
        )

    var_dict = _VarDict[Any](
        dtype=var.dtype,
        dims=var.dims,
        shape=var.shape,
        values=_normalize_var_values(var),
        variances=(
            None if var.variances is None else _normalize_var_values(var, 'variances')
        ),
        unit=var.unit,
        aligned=var.aligned,
        folding_required=(var.dtype in _FOLDED_DTYPES and var.ndim > 1),
    )
    return _VariableSerialization(
        (_SCIPP_VERSION, (False, var_dict, inst_dict)),
    )


def _unpickle_variable(var: _VariableSerialization) -> Variable:
    is_binned_and_kwargs_and_attrs = _check_and_strip_version(var)

    # Note: indexing the result tuple is clunky, but it helps `mypy`
    # with narrowing (see `mypy` issue #13218)
    if is_binned_and_kwargs_and_attrs[0]:  # Binned
        var_obj = bins(**is_binned_and_kwargs_and_attrs[1])
    else:  # Regular
        var_obj = _var_dict_to_var(is_binned_and_kwargs_and_attrs[1])

    # Restore free (user-assigned) attributes, if any
    for attr, value in is_binned_and_kwargs_and_attrs[2].items():
        setattr(var_obj, attr, value)

    return var_obj


def _pickle_array(da: DataArray) -> _DataArraySerialization:
    da_dict = _ArrayDict(
        data=da.data,
        coords=dict(da.coords),
        masks=dict(da.masks),
        name=da.name,
    )
    return _DataArraySerialization((_SCIPP_VERSION, da_dict))


def _unpickle_array(da: _DataArraySerialization) -> DataArray:
    return DataArray(**_check_and_strip_version(da))


def _pickle_dataset(ds: Dataset) -> _DatasetSerialization:
    data: dict[str, tuple[Variable, dict[str, Variable]]]
    data = {name: (da.data, dict(da.masks)) for name, da in ds.items()}
    ds_dict = _DatasetDict(data=data, coords=dict(ds.coords))
    return _DatasetSerialization((_SCIPP_VERSION, ds_dict))


def _unpickle_dataset(ds: _DatasetSerialization) -> Dataset:
    ds_dict = _check_and_strip_version(ds)
    data: dict[str, Variable | DataArray] = {}
    coords = ds_dict['coords']
    for name, (var, masks) in ds_dict['data'].items():
        if masks:
            data[name] = DataArray(data=var, coords=coords, masks=masks)
        else:
            data[name] = var
    return Dataset(data=data, coords=coords)


_register(Unit, _pickle_unit, _unpickle_unit)
_register(DType, _pickle_dtype, _unpickle_dtype)
_register(Variable, _pickle_variable, _unpickle_variable)
_register(DataArray, _pickle_array, _unpickle_array)
_register(Dataset, _pickle_dataset, _unpickle_dataset)
