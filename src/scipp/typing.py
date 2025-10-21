# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet
from __future__ import annotations

import typing as _std_typing
from collections.abc import Sequence

import numpy as np
import numpy.typing as npt

from ._scipp import core as sc
from .core.cpp_classes import DataArray, Dataset, DType, Variable
from .core.data_group import DataGroup


def is_scalar(obj: VariableLike) -> bool:
    """
    Return True if the input is a scalar.
    """
    return obj.ndim == 0


def has_vector_type(obj: Variable | DataArray) -> bool:
    """
    Return True if the object dtype is vector3.
    """
    return obj.dtype == sc.DType.vector3  # type: ignore[no-any-return]


def has_string_type(obj: Variable | DataArray) -> bool:
    """
    Return True if the object dtype is string.
    """
    return obj.dtype == sc.DType.string  # type: ignore[no-any-return]


def has_datetime_type(obj: Variable | DataArray) -> bool:
    """
    Return True if the object dtype is datetime64.
    """
    return obj.dtype == sc.DType.datetime64  # type: ignore[no-any-return]


def has_numeric_type(obj: Variable | DataArray) -> bool:
    """
    Return False if the dtype is either vector or string.
    """
    return (not has_vector_type(obj)) and (not has_string_type(obj))


Dims: _std_typing.TypeAlias = str | _std_typing.Iterable[str] | None
"""Describes dimensions to operate on.

Can be a string (for a single dimension)
or an iterable of strings (multiple dimensions).
A value of ``None`` indicates "all dimensions."
"""

VariableLike: _std_typing.TypeAlias = (
    Variable | DataArray | Dataset | DataGroup[_std_typing.Any]
)
"""Any object that behaves like a :class:`scipp.Variable`.

More concretely, an array with labeled dimensions which supports slicing and
arithmetic:

- :class:`scipp.DataArray`
- :class:`scipp.DataGroup`
- :class:`scipp.Dataset`
- :class:`scipp.Variable`
"""

MetaDataMap: _std_typing.TypeAlias = _std_typing.MutableMapping[str, Variable]
"""dict-like object mapping dimension labels to Variables."""

VariableLikeType = _std_typing.TypeVar(
    'VariableLikeType', Variable, DataArray, Dataset, DataGroup[_std_typing.Any]
)
"""TypeVar for use in annotations.

Should be hidden in rendered documentation in favor of VariableLike.
"""

DTypeLike: _std_typing.TypeAlias = npt.DTypeLike | DType
"""Anything that can be interpreted as a dtype.

This includes

- :class:`scipp.DType`
- everything that is supported by
  `numpy.DTypeLike <https://numpy.org/devdocs/reference/typing.html#numpy.typing.DTypeLike>`_
  e.g.

  - :class:`numpy.dtype`
  - :class:`type` objects like :class:`int` and :class:`float`
  - names of dtypes as strings like ``'int32'`` and ``'float64'``
"""

if _std_typing.TYPE_CHECKING:
    from enum import Enum

    class ellipsis(Enum):
        Ellipsis = "..."

else:
    ellipsis = type(Ellipsis)

ScippIndex: _std_typing.TypeAlias = (
    ellipsis
    | int
    | slice
    | Sequence[int]
    | npt.NDArray[np.integer[_std_typing.Any]]
    | tuple[
        str,
        int
        | slice
        | Sequence[int]
        | npt.NDArray[np.integer[_std_typing.Any]]
        | Variable,
    ]
    | Variable
)
