# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import typing as _std_typing

from .._scipp import core as sc


def is_variable(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, sc.Variable)


def is_scalar(obj):
    """
    Return True if the input is a scalar
    """
    return obj.dims == []


def is_dataset(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, sc.Dataset)


def is_data_array(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, sc.DataArray)


def is_dataset_or_array(obj):
    """
    Return True if the input object is either a Dataset or DataArray.
    """
    return is_dataset(obj) or is_data_array(obj)


def vector_type(obj):
    """
    Return True if the object dtype is vector_3_float64.
    """
    return obj.dtype == sc.dtype.vector_3_float64


def string_type(obj):
    """
    Return True if the object dtype is string.
    """
    return obj.dtype == sc.dtype.string


def datetime_type(obj):
    """
    Return True if the object dtype is datetime64.
    """
    return obj.dtype == sc.dtype.datetime64


def numeric_type(obj):
    """
    Return False if the dtype is either vector or string.
    """
    return (not vector_type(obj)) and (not string_type(obj))


DataArrayLike = _std_typing.Union[sc.Variable, sc.DataArray]
