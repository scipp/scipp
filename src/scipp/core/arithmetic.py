# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

from __future__ import annotations

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLike


def add(a: VariableLike, b: VariableLike) -> VariableLike:
    """Element-wise addition.

    Equivalent to

    .. code-block:: python

        a + b

    :param a: First summand.
    :param b: Second summand.
    :return: Sum of ``a`` and ``b``.

    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.add, a, b)


def divide(dividend: VariableLike, divisor: VariableLike) -> VariableLike:
    """Element-wise true division.

    This function corresponds to the ``__truediv__`` dunder method, i.e.
    ``dividend / divisor``.
    The result always contains fractional parts even when
    the inputs are integers:

    :param dividend: Dividend of the quotient.
    :param divisor: Divisor of the quotient.
    :return: Quotient.
    :seealso: :py:func:`scipp.floor_divide`

    Examples:

    >>> sc.divide(sc.arange('x', -2, 3), sc.scalar(2)).values
    array([-1. , -0.5,  0. ,  0.5,  1. ])
    >>> sc.divide(sc.arange('x', -2.0, 3.0), sc.scalar(2.0)).values
    array([-1. , -0.5,  0. ,  0.5,  1. ])

    Of equivalently in operator notation

    >>> (sc.arange('x', -2.0, 3.0) / sc.scalar(2.0)).values
    array([-1. , -0.5,  0. ,  0.5,  1. ])

    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.divide, dividend, divisor)


def floor_divide(dividend: VariableLike, divisor: VariableLike) -> VariableLike:
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
    array([-1, -1,  0,  0,  1])
    >>> sc.floor_divide(sc.arange('x', -2.0, 3.0), sc.scalar(2.0)).values
    array([-1., -1.,  0.,  0.,  1.])

    Of equivalently in operator notation

    >>> (sc.arange('x', -2.0, 3.0) // sc.scalar(2.0)).values
    array([-1., -1.,  0.,  0.,  1.])

    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.floor_divide, dividend, divisor)


def mod(dividend: VariableLike, divisor: VariableLike) -> VariableLike:
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
    array([0, 1, 2, 0, 1, 2, 0, 1])
    >>> sc.mod(sc.arange('x', -3, 5), sc.scalar(-3)).values
    array([ 0, -2, -1,  0, -2, -1,  0, -2])

    Of equivalently in operator notation

    >>> (sc.arange('x', -3, 5) % sc.scalar(3)).values
    array([0, 1, 2, 0, 1, 2, 0, 1])

    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.mod, dividend, divisor)


def multiply(a: VariableLike, b: VariableLike) -> VariableLike:
    """Element-wise product.

    Equivalent to

    .. code-block:: python

        a * b

    In cases where the order of the operands matters, e.g. with vectors
    or matrices, it is as shown above.

    :param a: Left factor
    :param b: Right factor.
    :return: Product of ``a`` and ``b``.

    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.multiply, a, b)


def subtract(minuend: VariableLike, subtrahend: VariableLike) -> VariableLike:
    """Element-wise difference.

    Equivalent to

    .. code-block:: python

        minuend - subtrahend

    :param minuend: Minuend.
    :param subtrahend: Subtrahend.
    :return: ``subtrahend`` subtracted from ``minuend``.

    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.subtract, minuend, subtrahend)
