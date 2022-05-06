# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import typing as _std_typing

import numpy.typing

from ._scipp import core as sc
from .core.cpp_classes import DataArray, Dataset, DType, Variable


def is_scalar(obj: _std_typing.Any) -> bool:
    """
    Return True if the input is a scalar.
    """
    return obj.ndim == 0


def has_vector_type(obj: _std_typing.Any) -> bool:
    """
    Return True if the object dtype is vector3.
    """
    return obj.dtype == sc.DType.vector3


def has_string_type(obj: _std_typing.Any) -> bool:
    """
    Return True if the object dtype is string.
    """
    return obj.dtype == sc.DType.string


def has_datetime_type(obj: _std_typing.Any) -> bool:
    """
    Return True if the object dtype is datetime64.
    """
    return obj.dtype == sc.DType.datetime64


def has_numeric_type(obj: _std_typing.Any) -> bool:
    """
    Return False if the dtype is either vector or string.
    """
    return (not has_vector_type(obj)) and (not has_string_type(obj))


VariableLike = _std_typing.Union[Variable, DataArray, Dataset]
"""Any object that behaves like a :class:`scipp.Variable`.

More concretely, an array with labeled dimensions:

- :class:`scipp.DataArray`
- :class:`scipp.Dataset`
- :class:`scipp.Variable`
"""

MetaDataMap = _std_typing.MutableMapping[str, Variable]
"""dict-like object mapping dimension labels to Variables."""

VariableLikeType = _std_typing.TypeVar('VariableLikeType', Variable, DataArray, Dataset)
"""TypeVar for use in annotations.

Should be hidden in rendered documentation in favor of VariableLike.
"""

DTypeLike = _std_typing.Union[numpy.typing.DTypeLike, DType]
"""Anything that can be interpreted as a dtype.

This includes

- :class:`scipp.DType`
- everything that is supported by
  `numpy.DTypeLike <https://numpy.org/devdocs/reference/typing.html#numpy.typing.DTypeLike>`_
  e.g.

  - :class:`numpy.dtype`
  - :class:`type` objects like :class:`int` and :class:`float`
  - names of dtypes as strings like ``'int32'`` and ``'float64'``
"""  # noqa: E501
