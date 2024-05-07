# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from .._scipp import core as _cpp
from ..typing import VariableLikeType
from ._cpp_wrapper_util import call_func as _call_cpp_func


def sinh(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
    """Element-wise hyperbolic sine.

    The input unit must be dimensionless.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The hyperbolic sine values of the input.
    """
    return _call_cpp_func(_cpp.sinh, x, out=out)


def cosh(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
    """Element-wise hyperbolic cosine.

    The input unit must be dimensionless.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The hyperbolic cosine values of the input.
    """
    return _call_cpp_func(_cpp.cosh, x, out=out)


def tanh(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
    """Element-wise hyperbolic tangent.

    The input unit must be dimensionless.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The hyperbolic tangent values of the input.
    """
    return _call_cpp_func(_cpp.tanh, x, out=out)


def asinh(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
    """Element-wise inverse hyperbolic sine.

    The input unit must be dimensionless.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The inverse hyperbolic sine values of the input.
    """
    return _call_cpp_func(_cpp.asinh, x, out=out)


def acosh(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
    """Element-wise inverse hyperbolic cosine.

    The input unit must be dimensionless.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The inverse hyperbolic cosine values of the input.
    """
    return _call_cpp_func(_cpp.acosh, x, out=out)


def atanh(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
    """Element-wise inverse hyperbolic tangent.

    The input unit must be dimensionless.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as input
        The inverse hyperbolic tangent values of the input.
    """
    return _call_cpp_func(_cpp.atanh, x, out=out)
