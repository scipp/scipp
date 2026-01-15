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

    Examples
    --------
    Compute the hyperbolic sine:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[-1.0, 0.0, 1.0])
      >>> sc.sinh(x)
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [-1.1752, 0, 1.1752]

    The input must be dimensionless:

      >>> sc.sinh(sc.scalar(0.0))
      <scipp.Variable> ()    float64  [dimensionless]  0
    """
    return _call_cpp_func(_cpp.sinh, x, out=out)  # type: ignore[return-value]


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

    Examples
    --------
    Compute the hyperbolic cosine:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[-1.0, 0.0, 1.0])
      >>> sc.cosh(x)
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [1.54308, 1, 1.54308]

    Note that cosh is an even function, so ``cosh(-x) == cosh(x)``.
    """
    return _call_cpp_func(_cpp.cosh, x, out=out)  # type: ignore[return-value]


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

    Examples
    --------
    Compute the hyperbolic tangent:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[-1.0, 0.0, 1.0])
      >>> sc.tanh(x)
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [-0.761594, 0, 0.761594]

    The output is bounded in the range (-1, 1).
    """
    return _call_cpp_func(_cpp.tanh, x, out=out)  # type: ignore[return-value]


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

    Examples
    --------
    Compute the inverse hyperbolic sine:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[-1.0, 0.0, 1.0])
      >>> sc.asinh(x)
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [-0.881374, 0, 0.881374]

    The function is defined for all real values.
    """
    return _call_cpp_func(_cpp.asinh, x, out=out)  # type: ignore[return-value]


def acosh(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
    """Element-wise inverse hyperbolic cosine.

    The input unit must be dimensionless and values must be >= 1.

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

    Examples
    --------
    Compute the inverse hyperbolic cosine:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0])
      >>> sc.acosh(x)
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [0, 1.31696, 1.76275]

    The domain is [1, ∞), and acosh(1) = 0.
    """
    return _call_cpp_func(_cpp.acosh, x, out=out)  # type: ignore[return-value]


def atanh(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
    """Element-wise inverse hyperbolic tangent.

    The input unit must be dimensionless and values must be in (-1, 1).

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

    Examples
    --------
    Compute the inverse hyperbolic tangent:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[-0.5, 0.0, 0.5])
      >>> sc.atanh(x)
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [-0.549306, 0, 0.549306]

    The domain is (-1, 1), and atanh(0) = 0.
    """
    return _call_cpp_func(_cpp.atanh, x, out=out)  # type: ignore[return-value]
