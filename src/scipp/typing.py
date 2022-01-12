# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import typing as _std_typing

from ._scipp import core as sc


def is_scalar(obj: _std_typing.Any) -> bool:
    """
    Return True if the input is a scalar.
    """
    return obj.dims == []


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


#: Any object that behaves like a scipp.Variable,
#:  that is an array with labeled dimensions.
VariableLike = _std_typing.Union[sc.Variable, sc.DataArray, sc.Dataset]

#: dict-like object mapping dimension labels to Variables.
MetaDataMap = _std_typing.MutableMapping[str, sc.Variable]
