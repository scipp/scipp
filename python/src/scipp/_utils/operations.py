# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)

from .._scipp import core as _cpp
from . import is_variable


def midpoint(low, high):
    """Compute the point in the middle of an interval.

    :param low: Lower end of the interval.
    :param high: Upper end of the interval.
    :return: The equivalent of (low + high) / 2.
    """
    if is_variable(low) ^ is_variable(high):
        raise TypeError('Either no or both operands must be Variables.')
    if low.dtype != high.dtype:
        raise TypeError('The arguments of midpoint must have the same dtype, '
                        f'got {low.dtype}, {high.dtype}')

    def is_datetime(dtype):
        try:
            return dtype == _cpp.dtype.datetime64
        except TypeError:
            return dtype.name.startswith('datetime')

    if is_datetime(low.dtype):
        # Can handle datetime but less precise and susceptible to underlow.
        return low + (high - low) // 2
    # More precise but susceptible to overflow.
    return 0.5 * (low + high)
 
