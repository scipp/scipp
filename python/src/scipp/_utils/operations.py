# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)

from . import is_variable
from .is_type import is_datetime


def midpoint(*, low, high):
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

    if is_datetime(low.dtype):
        # Can handle datetime but less precise and susceptible to underflow.
        return low + (high - low) // 2
    # More precise but susceptible to overflow.
    return 0.5 * (low + high)
