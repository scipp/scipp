# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
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
