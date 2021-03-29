# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def dot(x, y):
    """Element-wise dot product.

    :param x: Left hand side operand.
    :param y: Right hand side operand.
    :type x: Variable
    :type y: Variable
    :raises: If the dtype of the input is not vector_3_float64.
    :return: The dot product of the input vectors.
    :rtype: Variable
    """
    return _call_cpp_func(_cpp.dot, x, y)


def issorted(x, dim, order='ascending'):
    """
    Check if the values of a variable are sorted in.
    If `order` is `ascending`, check if values are non-decreasing along `dim`.
    If `order` is `descending`, check if values are non-increasing along 'dim'.

    :param x: Variable to check.
    :param dim: Dimension along which order is checked.
    :param order: Optional Sorted order. Valid options are 'ascending' and
      'descending'. Default is 'ascending'.
    :type x: Variable
    :type dim: str
    :type order: str
    :return: True if the variable values are monotonously ascending or
      descending (depending on the requested order), False otherwise.
    :rtype: bool
    """
    return _call_cpp_func(_cpp.issorted, x, dim, order)


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
