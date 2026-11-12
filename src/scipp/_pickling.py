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
from typing import Any, NewType, NotRequired, TypedDict, cast

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
    bins: NotRequired[dict[str, Any]]


class _ArrayDict[T: Any](TypedDict):
    data: Variable
    coords: dict[str, Variable]
    masks: dict[str, Variable]
    name: str
    bins: NotRequired[dict[str, Any]]


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
    tuple[_Version, _VarDict[Any]],
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


def _normalize_var_values(var: Variable, field: str = 'values') -> Any:
    """Return the normalized :attr:`Variable.values`.

    This is mainly for dealing with:

    - Linear-algebra types (affine transforms etc.),

    - String/object arrays, and

    - :mod:`numpy` strings.

    Cavearts, particularly for the second:

    - The ``.values`` of string and object types are
      ``ElementArrayView``s, which cannot reliably be iterated over,
      despite its having a :meth:`.__len__` and a :meth:`.__iter__`, the
      latter of which doesn't seem to work.

    - They are also often backed by 1-D arrays even when
      multi-dimensional, because the constructor doesn't take
      multi-D arrays (save for the edge case below), and must be formed
      by :meth:`Variable.fold`-ing a flat instance.

    - The one exception to the multi-dimensional rule above is when the
      trailing dimensions all have size 1, for some reason.
    """
    data = getattr(var, field)

    # Linalg types are always array(-like)s.
    if var.dtype in (
        DType.affine_transform3,
        DType.linear_transform3,
        DType.rotation3,
        DType.translation3,
        DType.vector3,
    ):
        return np.array(data)

    # Just the value, nothing to look at here
    if not var.shape:
        return data

    # Object and string arrays are converted into `ElementArrayView`s by
    # the `Variable` constructor; reverse that
    if var.dtype in (DType.PyObject, DType.string):
        try:
            return np.array(data)
        except TypeError:
            pass
        # Edge case:
        # - `.__iter__()` over object array views
        #   (`ElementArrayView_PyObject`) is botched, despite
        #   `.__len__()` somehow working.
        # - The shape of object arrays can be true 1-D (e.g. `(3,)`) or
        #   quasi-1D (e.g. `(10, 1, 1)`), the latter of which is
        #   permitted by the `Variable` constructor, perhaps only
        #   coincidentally. In any case the following should do.
        if var.dtype != DType.PyObject:
            raise AssertionError(f'{var.dtype=!r}!=DType.PyObject')
        n = len(data)
        array = np.empty(n, dtype=object)
        for i in range(n):
            array[i, ...] = data[i]
        return array

    return data


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
        folding_required=(var.dtype == DType.PyObject and var.ndim > 1),
    )
    if var.is_binned:
        raise NotImplementedError('TODO: handle binned data')
        # var_dict['bins'] = dict(cast(Bins[Variable], var.bins).constituents)
    return _VariableSerialization((_SCIPP_VERSION, var_dict))


def _unpickle_variable(var: _VariableSerialization) -> Variable:
    var_dict = _check_and_strip_version(var)

    try:
        return bins(**var_dict.pop('bins'))
    except KeyError:  # Not binned
        pass

    if fold := var_dict['folding_required']:
        values = cast(np.ndarray, var_dict['values']).flatten()
        shape: tuple[int, ...] = (int(np.prod(var_dict['shape'])),)
        dims: tuple[str, ...] = ('_',)
        if (variances := var_dict['variances']) is not None:
            variances = cast(np.ndarray, variances).flatten()
    else:
        values = var_dict['values']
        shape = var_dict['shape']
        dims = var_dict['dims']
        variances = var_dict['variances']

    var_obj = empty(
        dtype=var_dict['dtype'],
        dims=dims,
        shape=shape,
        unit=var_dict['unit'],
        aligned=var_dict['aligned'],
        with_variances=variances is not None,
    )

    setattr(var_obj, 'values' if shape else 'value', values)
    if variances is not None:
        setattr(var_obj, 'variances' if shape else 'variance', variances)

    if fold:
        return var_obj.fold(
            '_',
            shape=var_dict['shape'],
            dims=var_dict['dims'],
        )

    return var_obj


def _pickle_array(da: DataArray) -> _DataArraySerialization:
    da_dict = _ArrayDict(
        data=da.data,
        coords=dict(da.coords),
        masks=dict(da.masks),
        name=da.name,
    )
    if da.is_binned:
        raise NotImplementedError('TODO: handle binned data')
        # da_dict['bins'] = dict(cast(Bins[DataArray], da.bins).constituents)
    return _DataArraySerialization((_SCIPP_VERSION, da_dict))


def _unpickle_array(da: _DataArraySerialization) -> DataArray:
    da_dict = _check_and_strip_version(da)
    try:  # TODO
        da_dict['data'] = bins(**da_dict.pop('bins'))
    except KeyError:  # Not binned
        pass
    return DataArray(
        data=da_dict['data'],
        coords=da_dict['coords'],
        masks=da_dict['masks'],
        name=da_dict['name'],
    )


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
