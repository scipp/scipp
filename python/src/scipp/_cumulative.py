# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def cumsum(a, dim=None, inclusive=True):
    """Return the cumulative sum along the specified dimension.

    :param a: Input data.
    :param dim: Optional dimension along which to calculate the sum. If not
                given, the cumulative sum along all dimensions is calculated.
    :param inclusive: If `False`, the ith element is not included in the
                      output. Defaults to `True`.
    :return: The cumulative sum of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.cumsum, a, inclusive=inclusive)
    else:
        return _call_cpp_func(_cpp.cumsum, a, dim, inclusive=inclusive)
