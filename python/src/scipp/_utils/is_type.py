# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

import numpy as np

from .._scipp import core as sc


def is_variable(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, sc.Variable) or isinstance(obj, sc.VariableView)


def is_dataset(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, sc.Dataset) or isinstance(obj, sc.DatasetView)


def is_data_array(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, sc.DataArray) or isinstance(obj, sc.DataArrayView)


def is_dataset_or_array(obj):
    """
    Return True if the input object is either a Dataset or DataArray.
    """
    return is_dataset(obj) or is_data_array(obj)


def is_discrete(obj):
    """
    Return True if the input object is a type that has a discrete domain.
    """
    return isinstance(obj, (int, np.datetime64)) or (isinstance(
        obj, np.ndarray) and obj.dtype not in (np.float32, np.float64)) or (
            is_variable(obj)
            and obj.dtype not in (sc.dtype.float32, sc.dtype.float64))


def is_datetime(dtype):
    """
    Return True if the input is a datetime64 dtype (scipp or numpy).
    """
    try:
        return dtype == sc.dtype.datetime64
    except TypeError:
        return dtype.name.startswith('datetime')
