# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import typing as _std_typing

from ._scipp import core as sc


def is_variable(obj: _std_typing.Any) -> bool:
    """
    Return True if the input is of type scipp.Variable.
    """
    return isinstance(obj, sc.Variable)


def is_scalar(obj: _std_typing.Any) -> bool:
    """
    Return True if the input is a scalar.
    """
    return obj.dims == []


def is_dataset(obj: _std_typing.Any) -> bool:
    """
    Return True if the input is of type scipp.Dataset.
    """
    return isinstance(obj, sc.Dataset)


def is_data_array(obj: _std_typing.Any) -> bool:
    """
    Return True if the input is of type scipp.DataArray.
    """
    return isinstance(obj, sc.DataArray)


def is_dataset_or_array(obj: _std_typing.Any) -> bool:
    """
    Return True if the input object is either a scipp.Dataset or DataArray.
    """
    return is_dataset(obj) or is_data_array(obj)


def has_vector_type(obj: _std_typing.Any) -> bool:
    """
    Return True if the object dtype is vector_3_float64.
    """
    return obj.dtype == sc.dtype.vector_3_float64


def has_string_type(obj: _std_typing.Any) -> bool:
    """
    Return True if the object dtype is string.
    """
    return obj.dtype == sc.dtype.string


def has_datetime_type(obj: _std_typing.Any) -> bool:
    """
    Return True if the object dtype is datetime64.
    """
    return obj.dtype == sc.dtype.datetime64


def has_numeric_type(obj: _std_typing.Any) -> bool:
    """
    Return False if the dtype is either vector or string.
    """
    return (not has_vector_type(obj)) and (not has_string_type(obj))


#: Any object that behaves like a scipp.DataArray in most operations.
#: This explicitly excludes Datasets.
DataArrayLike = _std_typing.Union[sc.Variable, sc.DataArray]

#: Any object that behaves like a scipp.Dataset in most operations.
DatasetLike = _std_typing.Union[DataArrayLike, sc.Dataset]
