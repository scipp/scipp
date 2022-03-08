# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen

from __future__ import annotations

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLike


def add(a: VariableLike, b: VariableLike) -> VariableLike:
    """Element-wise addition.

    Equivalent to::

        a + b

    Parameters
    ----------
    a :
        First summand.
    b :
        Second summand.


    Returns
    -------
    :
        Sum of ``a`` and ``b``.

    Note
    ----
    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """

    return _call_cpp_func(_cpp.add, a, b)


def divide(dividend: VariableLike, divisor: VariableLike) -> VariableLike:
    """Element-wise true division.

    Equivalent to::

        dividend / divisor

    The result always contains fractional parts even when
    the inputs are integers:

    Parameters
    ----------
    dividend :
        Dividend of the quotient.
    divisor :
        Divisor of the quotient.

    Returns
    -------
    :
        Quotient of ``divident`` and ``divisor``.

    Examples
    --------
    >>> sc.divide(sc.arange('x', -2, 3), sc.scalar(2)).values
    array([-1. , -0.5,  0. ,  0.5,  1. ])
    >>> sc.divide(sc.arange('x', -2.0, 3.0), sc.scalar(2.0)).values
    array([-1. , -0.5,  0. ,  0.5,  1. ])

    Of equivalently in operator notation

    >>> (sc.arange('x', -2.0, 3.0) / sc.scalar(2.0)).values
    array([-1. , -0.5,  0. ,  0.5,  1. ])

    See Also
    --------
    scipp.floor_divide

    Note
    ----
    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.divide, dividend, divisor)


def floor_divide(dividend: VariableLike, divisor: VariableLike) -> VariableLike:
    """Element-wise floor division.

    Equivalent to::

        dividend // divisor

    Parameters
    ----------
    dividend:
        Dividend of the quotient.
    divisor:
        Divisor of the quotient.

    Returns
    -------
    :
        Rounded down quotient.

    Examples
    --------

    >>> sc.floor_divide(sc.arange('x', -2, 3), sc.scalar(2)).values
    array([-1, -1,  0,  0,  1])
    >>> sc.floor_divide(sc.arange('x', -2.0, 3.0), sc.scalar(2.0)).values
    array([-1., -1.,  0.,  0.,  1.])

    Or equivalently in operator notation

    >>> (sc.arange('x', -2.0, 3.0) // sc.scalar(2.0)).values
    array([-1., -1.,  0.,  0.,  1.])

    See Also
    --------
    scipp.divide, scipp.mod

    Note
    ----
    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.floor_divide, dividend, divisor)


def mod(dividend: VariableLike, divisor: VariableLike) -> VariableLike:
    """Element-wise remainder.

    Equivalent to::

        dividend % divisor

    In scipp, the remainder is defined to complement
    :py:func:`scipp.floor_divide` meaning that::

        a == floor(a / b) * b + a % b

    This is the same definition as in :py:data:`numpy.mod`
    and :py:data:`numpy.remainder`.

    Warning
    -------
    This differs from the IEEE 754 remainder as implemented by
    :py:func:`math.remainder` and C's remainder function and modulus
    operator, which complement ``round(a / b)``.

    Parameters
    ----------
    dividend:
        Dividend of the quotient.
    divisor:
        Divisor of the quotient.

    Returns
    -------
    :
        Quotient.

    Examples
    --------

    >>> sc.mod(sc.arange('x', -3, 5), sc.scalar(3)).values
    array([0, 1, 2, 0, 1, 2, 0, 1])
    >>> sc.mod(sc.arange('x', -3, 5), sc.scalar(-3)).values
    array([ 0, -2, -1,  0, -2, -1,  0, -2])

    Or equivalently in operator notation

    >>> (sc.arange('x', -3, 5) % sc.scalar(3)).values
    array([0, 1, 2, 0, 1, 2, 0, 1])

    See Also
    --------
    scipp.floor_divide, scipp.divide

    Note
    ----
    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.mod, dividend, divisor)


def multiply(left: VariableLike, right: VariableLike) -> VariableLike:
    """Element-wise product.

    Equivalent to::

        left * right

    In cases where the order of the operands matters, e.g. with vectors
    or matrices, it is as shown above.

    Parameters
    ----------
    left:
        Left factor
    right:
        Right factor.

    Returns
    -------
    :
        Product of ``left`` and ``right``.

    Note
    ----
    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.multiply, left, right)


def negative(a) -> VariableLike:
    """Element-wise negative.

    Equivalent to::

        -a

    Parameters
    ----------
    a:
        Input data.

    Returns
    -------
    :
        ``a`` with flipped signs.
    """
    return _call_cpp_func(_cpp.negative, a)


def subtract(minuend: VariableLike, subtrahend: VariableLike) -> VariableLike:
    """Element-wise difference.

    Equivalent to::

        minuend - subtrahend

    Parameters
    ----------
    minuend:
        Minuend.
    subtrahend:
        Subtrahend.

    Returns
    -------
    :
        ``subtrahend`` subtracted from ``minuend``.

    Notes
    -----
    See the guide on `computation <../../user-guide/computation.rst>`_ for
    general concepts and broadcasting behavior.
    """
    return _call_cpp_func(_cpp.subtract, minuend, subtrahend)
