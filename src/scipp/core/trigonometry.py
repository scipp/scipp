# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from __future__ import annotations

from .._scipp import core as _cpp
from ..typing import VariableLikeType
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .cpp_classes import Variable


def sin(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
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

    Examples
    --------
    Compute sine of angles in radians:

      >>> import scipp as sc
      >>> import numpy as np
      >>> angle = sc.array(dims=['x'], values=[0.0, np.pi/6, np.pi/4, np.pi/3, np.pi/2], unit='rad')
      >>> sc.sin(angle)
      <scipp.Variable> (x: 5)    float64  [dimensionless]  [0, 0.5, ..., 0.866025, 1]

    The input must have angle units (rad or deg):

      >>> angle_deg = sc.array(dims=['x'], values=[0.0, 30.0, 45.0, 60.0, 90.0], unit='deg')
      >>> sc.sin(angle_deg)
      <scipp.Variable> (x: 5)    float64  [dimensionless]  [0, 0.5, ..., 0.866025, 1]
    """
    return _call_cpp_func(_cpp.sin, x, out=out)  # type: ignore[return-value]


def cos(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
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

    Examples
    --------
    Compute cosine of angles in radians:

      >>> import scipp as sc
      >>> import numpy as np
      >>> angle = sc.array(dims=['x'], values=[0.0, np.pi/6, np.pi/3, np.pi/2], unit='rad')
      >>> sc.cos(angle)
      <scipp.Variable> (x: 4)    float64  [dimensionless]  [1, 0.866025, 0.5, ...]

    The output is dimensionless.
    """
    return _call_cpp_func(_cpp.cos, x, out=out)  # type: ignore[return-value]


def tan(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
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

    Examples
    --------
    Compute tangent of angles:

      >>> import scipp as sc
      >>> angle = sc.array(dims=['x'], values=[0.0, 30.0, 45.0], unit='deg')
      >>> sc.tan(angle)
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [0, 0.57735, 1]

    The input must have angle units (rad or deg).
    The output is dimensionless.
    """
    return _call_cpp_func(_cpp.tan, x, out=out)  # type: ignore[return-value]


def asin(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
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
    return _call_cpp_func(_cpp.asin, x, out=out)  # type: ignore[return-value]


def acos(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
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
    return _call_cpp_func(_cpp.acos, x, out=out)  # type: ignore[return-value]


def atan(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
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
    return _call_cpp_func(_cpp.atan, x, out=out)  # type: ignore[return-value]


def atan2(*, y: Variable, x: Variable, out: Variable | None = None) -> Variable:
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
        The equivalent in NumPy with additional explanations.
    """
    return _call_cpp_func(_cpp.atan2, y=y, x=x, out=out)  # type: ignore[return-value]
