# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from __future__ import annotations
from typing import Optional, Union

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLike


def dot(x: VariableLike, y: VariableLike) -> VariableLike:
    """Element-wise dot product.

    :param x: Left hand side operand.
    :param y: Right hand side operand.
    :raises: If the dtype of the input is not vector_3_float64.
    :return: The dot product of the input vectors.
    """
    return _call_cpp_func(_cpp.dot, x, y)


def islinspace(x: _cpp.Variable, dim: str = None) -> _cpp.Variable:
    """
    Check if the values of a variable are evenly spaced.

    :param x: Variable to check.
    :param dim: Optional variable for the dim to check from the Variable.
    :returns: Variable of value True if the variable contains regularly
     spaced values, variable of value False otherwise.
    """
    if dim is None:
        return _call_cpp_func(_cpp.islinspace, x)
    else:
        return _call_cpp_func(_cpp.islinspace, x, dim)


def issorted(x: _cpp.Variable,
             dim: str,
             order: Optional[str] = 'ascending') -> VariableLike:
    """
    Check if the values of a variable are sorted.

    - If ``order`` is 'ascending',
      check if values are non-decreasing along ``dim``.
    - If ``order`` is 'descending',
      check if values are non-increasing along ``dim``.

    :param x: Variable to check.
    :param dim: Dimension along which order is checked.
    :param order: Sorting order. Valid options are 'ascending' and
      'descending'. Default is 'ascending'.
    :return: Variable containing one less dim, than the original
     variable with the corresponding boolean value for whether or
     not it was sorted along the given dim for the other
     dimensions.
    """
    return _call_cpp_func(_cpp.issorted, x, dim, order)


def allsorted(x: _cpp.Variable, dim: str, order: Optional[str] = 'ascending') -> bool:
    """
    Check if the values of a variable are sorted.

    - If ``order`` is 'ascending',
      check if values are non-decreasing along ``dim``.
    - If ``order`` is 'descending',
      check if values are non-increasing along ``dim``.

    :param x: Variable to check.
    :param dim: Dimension along which order is checked.
    :param order: Sorting order. Valid options are 'ascending' and
      'descending'. Default is 'ascending'.
    :return: True if the variable values are monotonously ascending or
      descending (depending on the requested order), False otherwise.
    """
    return _call_cpp_func(_cpp.allsorted, x, dim, order)


def cross(x: VariableLike, y: VariableLike) -> VariableLike:
    """Element-wise cross product.

    :param x: Left hand side operand.
    :param y: Right hand side operand.
    :raises: If the dtype of the input is not vector_3_float64.
    :return: The cross product of the input vectors.
    """
    return _call_cpp_func(_cpp.cross, x, y)


def sort(x: VariableLike,
         key: Union[str, _cpp.Variable],
         order: Optional[str] = 'ascending') -> VariableLike:
    """Sort variable along a dimension by a sort key or dimension label

    - If ``order`` is 'ascending',
      sort such that values are non-decreasing according to ``key``.
    - If ``order`` is 'descending',
      sort such that values are non-increasing according to ``key``.

    :param x: Data to be sorted.
    :param key: Either a 1D variable sort key or a dimension label.
    :param order: Sorting order. Valid options are 'ascending' and
      'descending'. Default is 'ascending'.
    :raises: If the key is invalid, e.g., if it does not have
      exactly one dimension, or if its dtype is not sortable.
    :return: The sorted equivalent of the input.
    """
    return _call_cpp_func(_cpp.sort, x, key, order)


def values(x: VariableLike) -> VariableLike:
    """Return the object without variances.

    :param x: Variable or DataArray
    :seealso: :py:func:`scipp.variances`, :py:func:`scipp.stddevs`.
    """
    return _call_cpp_func(_cpp.values, x)


def variances(x: VariableLike) -> VariableLike:
    """Return object containing the variances of the input as values.

    :param x: Variable or DataArray
    :seealso: :py:func:`scipp.values`, :py:func:`scipp.stddevs`.
    """
    return _call_cpp_func(_cpp.variances, x)


def stddevs(x: VariableLike) -> VariableLike:
    """Return object containing the stddevs of the input as values.

    This is essentially `sqrt(variances(x))`

    :param x: Variable or DataArray
    :seealso: :py:func:`scipp.values`, :py:func:`scipp.stddevs`.
    """
    return _call_cpp_func(_cpp.stddevs, x)


def rebin(x: VariableLike, dim: str, bins: _cpp.Variable) -> VariableLike:
    """
    Rebin a dimension of a data array or dataset.

    The input must contain bin edges for the given dimension `dim`.

    :param x: Data to rebin.
    :param dim: Dimension to rebin over.
    :param bins: New bin edges.
    :raises: If data cannot be rebinned, e.g., if the existing coordinate is not a
             bin-edge coordinate.
    :return: Data rebinned according to the new bin edges.
    """
    return _call_cpp_func(_cpp.rebin, x, dim, bins)


def where(condition: _cpp.Variable, x: _cpp.Variable,
          y: _cpp.Variable) -> _cpp.Variable:
    """Return elements chosen from x or y depending on condition.

    :param condition: Variable with dtype=bool. Where True, yield x, otherwise yield y.
    :param x: Variable with values from which to choose.
    :param y: Variable with values from which to choose.
    :return: Variable with elements from x where condition is True, and elements from y
             elsewhere.
    :seealso: :py:func:`scipp.choose`
    """
    return _call_cpp_func(_cpp.where, condition, x, y)
