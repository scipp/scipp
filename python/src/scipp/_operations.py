# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def sort(x, key, order='ascending'):
    """Sort variable along a dimension by a sort key or dimension label

    :param x: Data to be sorted.
    :param key: Either a 1D variable sort key or a dimension label.
    :param order: Optional Sorted order. Valid options are 'ascending' and
      'descending'. Default is 'ascending'.
    :type x: Dataset, DataArray, Variable
    :type key: Variable, str
    :type order: str
    :raises: If the key is invalid, e.g., if it does not have
      exactly one dimension, or if its dtype is not sortable.
    :return: The sorted equivalent of the input.
    """
    return _call_cpp_func(_cpp.sort, x, key, order)


def values(x):
    """Return the object without variances.

    :param x: Variable or DataArray
    :seealso: :py:func:`scipp.variances`, :py:func:`scipp.stddevs`.
    """
    return _call_cpp_func(_cpp.values, x)


def variances(x):
    """Return object containing the variances of the input as values.

    :param x: Variable or DataArray
    :seealso: :py:func:`scipp.values`, :py:func:`scipp.stddevs`.
    """
    return _call_cpp_func(_cpp.variances, x)


def stddevs(x):
    """Return object containing the stddevs of the input as values.

    This is essentially `sqrt(variances(x))`

    :param x: Variable or DataArray
    :seealso: :py:func:`scipp.values`, :py:func:`scipp.stddevs`.
    """
    return _call_cpp_func(_cpp.stddevs, x)
