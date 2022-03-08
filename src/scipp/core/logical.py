# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Jan-Lukas Wynen
from __future__ import annotations

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLike


def logical_not(x: VariableLike) -> VariableLike:
    """Element-wise logical negation.

    Equivalent to::

        ~a

    Parameters
    ----------
    x:
        Input data.

    Returns
    -------
    :
        The logical inverse of ``x``.
    """
    return _call_cpp_func(_cpp.logical_not, x)


def logical_and(a: VariableLike, b: VariableLike) -> VariableLike:
    """Element-wise logical and.

    Equivalent to::

        a & b

    Parameters
    ----------
    a:
        First input.
    b:
        Second input.

    Returns
    -------
    :
        The logical and of the elements of ``a`` and ``b``.
    """
    return _call_cpp_func(_cpp.logical_and, a, b)


def logical_or(a: VariableLike, b: VariableLike) -> VariableLike:
    """Element-wise logical or.

    Equivalent to::

        a | b

    Parameters
    ----------
    a:
        First input.
    b:
        Second input.

    Returns
    -------
    :
        The logical or of the elements of ``a`` and ``b``.
    """
    return _call_cpp_func(_cpp.logical_or, a, b)


def logical_xor(a: VariableLike, b: VariableLike) -> VariableLike:
    """Element-wise logical exclusive-or.

    Equivalent to::

        a ^ b

    Parameters
    ----------
    a:
        First input.
    b:
        Second input.

    Returns
    -------
    :
        The logical exclusive-or of the elements of ``a`` and ``b``.
    """
    return _call_cpp_func(_cpp.logical_xor, a, b)
