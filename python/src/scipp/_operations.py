# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew

from ._cpp_wrapper_util import call_func as _call_cpp_func
from ._scipp import core as _cpp
from . import Variable
from ._utils import is_variable


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


def midpoint(low, high):
    """Compute the point in the middle of an interval.

    :param low: Lower end of the interval.
    :param high: Upper end of the interval.
    :return: The equivalent of (low + high) / 2.
    """
    if low.dtype != high.dtype:
        raise TypeError(f'The arguments of midpoint must have the same dtype, got {low.dtype}, {high.dtype}')

    if is_variable(low):
        if not is_variable(high):
            raise TypeError('Either no or both operands must be Variables.')
        if low.dtype == _cpp.dtype.datetime64:
            diff = (high - low).values.astype(int, copy=False)
            return low + Variable(dims=low.dims, unit=low.unit, values=diff // 2)
    else:
        if low.dtype.name.startswith('datetime'):
            return low + (high - low) // 2

    # If dtype != datetime, we don't need to know the type of low and high.
    return 0.5 * (low + high)
