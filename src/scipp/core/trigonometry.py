# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from __future__ import annotations
from typing import Optional

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLikeType


def sin(x: VariableLikeType,
        *,
        out: Optional[VariableLikeType] = None) -> VariableLikeType:
    """Element-wise sine.

    The input must have a plane-angle unit, i.e. ``rad``, ``deg``.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The sine values of the input.
    """
    return _call_cpp_func(_cpp.sin, x, out=out)


def cos(x: VariableLikeType,
        *,
        out: Optional[VariableLikeType] = None) -> VariableLikeType:
    """Element-wise cosine.

    The input must have a plane-angle unit, i.e. ``rad``, ``deg``.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The cosine values of the input.
    """
    return _call_cpp_func(_cpp.cos, x, out=out)


def tan(x: VariableLikeType,
        *,
        out: Optional[VariableLikeType] = None) -> VariableLikeType:
    """Element-wise tangent.

    The input must have a plane-angle unit, i.e. ``rad``, ``deg``.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The tangent values of the input.
    """
    return _call_cpp_func(_cpp.tan, x, out=out)


def asin(x: VariableLikeType,
         *,
         out: Optional[VariableLikeType] = None) -> VariableLikeType:
    """Element-wise inverse sine.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The inverse sine values of the input in radians.
    """
    return _call_cpp_func(_cpp.asin, x, out=out)


def acos(x: VariableLikeType,
         *,
         out: Optional[VariableLikeType] = None) -> VariableLikeType:
    """Element-wise inverse cosine.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The inverse cosine values of the input in radians.
    """
    return _call_cpp_func(_cpp.acos, x, out=out)


def atan(x: VariableLikeType,
         *,
         out: Optional[VariableLikeType] = None) -> VariableLikeType:
    """Element-wise inverse tangent.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The inverse tangent values of the input in radians.
    """
    return _call_cpp_func(_cpp.atan, x, out=out)


def atan2(*,
          y: _cpp.Variable,
          x: _cpp.Variable,
          out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Element-wise inverse tangent of y/x determining the correct quadrant.

    Parameters
    ----------
    y:
        Input y values.
    x:
        Input x values.
    out:
        Optional output buffer.

    Returns
    -------
    :
        The signed inverse tangent values of y/x in the range [-π, π].

    See Also
    --------
    `<https://en.cppreference.com/w/c/numeric/math/atan2>`_:
        Documentation of all edge cases.
        Note that domain errors are *not* propagated to Python.
    numpy.arctan2:
        The equivalent in numpy with additional explanations.
    """
    return _call_cpp_func(_cpp.atan2, y=y, x=x, out=out)
