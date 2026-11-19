# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Terence Tsang

"""Basic implementation for (un-)pickling :mod:`scipp` objects.

This is mainly for the benefit of e.g. interoperability with
:mod:`multiprocessing`.

Caveats
-------
- This is NOT meant for persisting data; users are directed to either
  :mod:`scippnexus` or the HDF5-based scheme (see e.g.
  :func:`scipp.io.save_hdf5` and :func:`scipp.io.load_hdf5`) for that
  purpose.

- There is no special provision for subclasses.

- The pickle serialization should be considered opaque; the schemas'
  consistency should NOT be relied upon. In fact, attempting to unpickle
  a pickle prepared by another :mod:`scipp` version will result in a
  :class:`RuntimeError`.
"""

from __future__ import annotations

from collections.abc import Callable
from copyreg import pickle
from typing import Any, cast

import numpy as np

from . import __version__ as _VERSION
from .core import (
    DataArray,
    Dataset,
    DType,
    Unit,
    Variable,
    bins,
    empty,
)
from .utils.serialization_commons import should_reduce_binned_data

_FOLDED_DTYPES = (  # These cannot be natively multi-dimensional.
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


# -------------------------- Helper functions --------------------------


def _check_version(version: str) -> None:
    if version == _VERSION:
        return
    raise RuntimeError(
        'version mismatch: '
        f'`scipp` package version = {_VERSION!r}, pickled {version = !r}; '
        'note that pickling is only meant for processes '
        '(e.g. `multiprocessing` workers) using the same Python and '
        'the same `scipp`, and users should use e.g. `.save_hdf5()` '
        'for actual data storage.'
    )


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

    - They are in most cases often backed by 1-D arrays even when
      multi-dimensional, because the `Variable` constructor doesn't take
      multi-D arrays (save for the corner case of quasi 1-D arrays, e.g.
      `.shape=(10, 1, 1)`; see however issue #4018), and thus they must
      be :meth:`Variable.fold`-ed into shape from a flat instance.
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
) -> dict[str, Any]:
    if should_reduce_binned_data(binned.bins, reduction_threshold):
        binned = binned.copy()
    return dict(binned.bins.constituents)


# ------------------------- Pickling functions -------------------------


def _pickle_unit(unit: Unit) -> tuple[Callable[..., Unit], tuple[Any, ...]]:
    return _unpickle_unit, (_VERSION, unit.to_dict())


def _unpickle_unit(version: str, unit: dict[str, Any]) -> Unit:
    _check_version(version)
    return Unit.from_dict(unit)


def _pickle_dtype(dtype: DType) -> tuple[Callable[..., DType], tuple[Any, ...]]:
    return _unpickle_dtype, (_VERSION, str(dtype))


def _unpickle_dtype(version: str, dtype: str) -> DType:
    _check_version(version)
    return cast(DType, getattr(DType, dtype))


def _pickle_variable(
    var: Variable,
) -> tuple[Callable[..., Variable], tuple[Any, ...]]:
    if var.is_binned:
        constituents = _reduce_bins_constituents(var)
        return _unpickle_binned_variable, (_VERSION, constituents)
    if (variances := var.variances) is not None:
        variances = _normalize_var_values(var, 'variances')
    args = (
        _VERSION,
        var.dtype,
        var.dims,
        var.shape,
        _normalize_var_values(var),
        variances,
        var.unit,
        var.aligned,
    )
    return _unpickle_unbinned_variable, args


def _unpickle_unbinned_variable(
    version: str,
    dtype: DType,
    dims: tuple[str, ...],
    shape: tuple[int, ...],
    values: Any,
    variances: Any | None,
    unit: Unit | None,
    aligned: bool,
) -> Variable:
    _check_version(version)

    refold_kwargs: dict[str, Any] = {}
    if dtype in _FOLDED_DTYPES and len(shape) > 1:
        values = cast(np.ndarray, values).flatten()
        refold_kwargs['shape'], shape = shape, (int(np.prod(shape)),)
        refold_kwargs['dims'], dims = dims, ('_',)
        if variances is not None:
            variances = cast(np.ndarray, variances).flatten()

    var = empty(
        dtype=dtype,
        dims=dims,
        shape=shape,
        unit=unit,
        aligned=aligned,
        with_variances=variances is not None,
    )

    setattr(var, 'values' if shape else 'value', values)
    if variances is not None:
        setattr(var, 'variances' if shape else 'variance', variances)

    if refold_kwargs:
        return var.fold('_', **refold_kwargs)
    return var


def _unpickle_binned_variable(
    version: str,
    constituents: dict[str, Any],
) -> Variable:
    _check_version(version)
    return bins(**constituents)


def _pickle_array(
    da: DataArray,
) -> tuple[Callable[..., DataArray], tuple[Any, ...]]:
    args = (_VERSION, da.data, dict(da.coords), dict(da.masks), da.name)
    return _unpickle_array, args


def _unpickle_array(
    version: str,
    data: Variable,
    coords: dict[str, Variable],
    masks: dict[str, Variable],
    name: str,
) -> DataArray:
    _check_version(version)
    return DataArray(data=data, coords=coords, masks=masks, name=name)


def _pickle_dataset(
    ds: Dataset,
) -> tuple[Callable[..., Dataset], tuple[Any, ...]]:
    data = {name: (da.data, dict(da.masks)) for name, da in ds.items()}
    return _unpickle_dataset, (_VERSION, data, dict(ds.coords))


def _unpickle_dataset(
    version: str,
    data: dict[str, tuple[Variable, dict[str, Variable]]],
    coords: dict[str, Variable],
) -> Dataset:
    _check_version(version)

    repacked: dict[str, Variable | DataArray] = {}
    for name, (var, masks) in data.items():
        if masks:
            repacked[name] = DataArray(data=var, coords=coords, masks=masks)
        else:
            repacked[name] = var
    return Dataset(data=repacked, coords=coords)


pickle(Unit, _pickle_unit)
pickle(DType, _pickle_dtype)
pickle(Variable, _pickle_variable)
pickle(DataArray, _pickle_array)
pickle(Dataset, _pickle_dataset)
