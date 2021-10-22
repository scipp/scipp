# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from typing import Optional

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def cumsum(a: _cpp.Variable,
           dim: Optional[str] = None,
           mode: Optional[str] = 'inclusive') -> _cpp.Variable:
    """Return the cumulative sum along the specified dimension.

    See :py:func:`scipp.sum` on how rounding errors for float32 inputs are handled.

    :param a: Input data.
    :param dim: Optional dimension along which to calculate the sum. If not
                given, the cumulative sum along all dimensions is calculated.
    :param mode: Include or exclude the ith element (along dim) in the sum.
                 Options are 'inclusive' and 'exclusive'. Defaults to
                 'inclusive'.
    :return: The cumulative sum of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.cumsum, a, mode=mode)
    else:
        return _call_cpp_func(_cpp.cumsum, a, dim, mode=mode)
