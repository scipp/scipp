# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

from __future__ import annotations

from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .typing import DataArrayLike


def divide(dividend: DataArrayLike, divisor: DataArrayLike) -> DataArrayLike:
    """Element-wise true division.

    This function corresponds to the ``__truediv__`` dunder method, i.e.
    ``dividend / divisor``.
    The result always contains factional parts even when
    the inputs are integers:

    :param dividend: Dividend of the quotient.
    :param divisor: Divisor of the quotient.
    :return: Quotient.
    :seealso: :py:func:`scipp.floor_divide`

    Examples:

    >>> sc.divide(sc.arange('x', -2, 3), sc.scalar(2)).values
    [-1.  -0.5  0.   0.5  1. ]
    >>> sc.divide(sc.arange('x', -2.0, 3.0), sc.scalar(2.0)).values
    [-1.  -0.5  0.   0.5  1. ]

    Of equivalently in operator notation

    >>> (sc.arange('x', -2.0, 3.0) / sc.scalar(2.0)).values
    [-1.  -0.5  0.   0.5  1. ]
    """
    return _call_cpp_func(_cpp.divide, dividend, divisor)


def floor_divide(dividend: DataArrayLike,
                 divisor: DataArrayLike) -> DataArrayLike:
    """Element-wise floor division.

    This function corresponds to the ``__floordiv__`` dunder method, i.e.
    ``dividend // divisor``.
    The result is rounded down:

    :param dividend: Dividend of the quotient.
    :param divisor: Divisor of the quotient.
    :return: Rounded down quotient.
    :seealso: :py:func:`scipp.divide`, :py:func:`scipp.mod`

    Examples:

    >>> sc.floor_divide(sc.arange('x', -2, 3), sc.scalar(2)).values
    [-1 -1  0  0  1]
    >>> sc.floor_divide(sc.arange('x', -2.0, 3.0), sc.scalar(2.0)).values
    [-1. -1.  0.  0.  1.]

    Of equivalently in operator notation

    >>> (sc.arange('x', -2.0, 3.0) // sc.scalar(2.0)).values
    [-1. -1.  0.  0.  1.]
    """
    return _call_cpp_func(_cpp.floor_divide, dividend, divisor)


def mod(dividend: DataArrayLike, divisor: DataArrayLike) -> DataArrayLike:
    """Element-wise remainder.

    This function corresponds to the modulus operator ``dividend % divisor``.

    In scipp, the remainder is defined to complement
    :py:func:`scipp.floor_divide` meaning that

    .. code-block:: python

        a == floor(a / b) * b + a % b

    This is the same definition as in :py:data:`numpy.mod`
    and :py:data:`numpy.remainder`.

    .. warning::

        This differs from the IEEE 754 remainder as implemented by
        :py:func:`math.remainder` and C's remainder function and modulus
        operator, which complement ``round(a / b)``.

    :param dividend: Dividend of the quotient.
    :param divisor: Divisor of the quotient.
    :return: Quotient.
    :seealso: :py:func:`scipp.floor_divide`, :py:func:`scipp.divide`

    Examples:

    >>> sc.mod(sc.arange('x', -3, 5), sc.scalar(3)).values
    [0 1 2 0 1 2 0 1]
    >>> sc.mod(sc.arange('x', -3, 5), sc.scalar(-3)).values
    [ 0 -2 -1  0 -2 -1  0 -2]

    Of equivalently in operator notation

    >>> (sc.arange('x', -3, 5) % sc.scalar(3)).values
    [0 1 2 0 1 2 0 1]
    """
    return _call_cpp_func(_cpp.mod, dividend, divisor)
