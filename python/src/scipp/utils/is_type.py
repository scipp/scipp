# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @file
# @author Neil Vaytet

from .._scipp.core import (Variable, VariableView, Dataset, DatasetView,
                           DataArray, DataArrayView, contains_events)


def is_variable(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, Variable) or isinstance(obj, VariableView)


def is_dataset(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, Dataset) or isinstance(obj, DatasetView)


def is_data_array(obj):
    """
    Return True if the input is of type scipp.core.Variable.
    """
    return isinstance(obj, DataArray) or isinstance(obj, DataArrayView)


def is_dataset_or_array(obj):
    """
    Return True if the input object is either a Dataset or DataArray.
    """
    return is_dataset(obj) or is_data_array(obj)


def is_data_events(obj):
    if hasattr(obj, "data"):
        if obj.data is None:
            return False
        return contains_events(obj.data)
    return contains_events(obj)
