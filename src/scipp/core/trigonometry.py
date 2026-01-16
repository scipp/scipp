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
    """  # noqa: E501
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
    """  # noqa: E501
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

    The input must be dimensionless and in the range [-1, 1].

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

    Examples
    --------
    Compute the inverse sine:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[0.0, 0.5, 1.0])
      >>> sc.asin(x)
      <scipp.Variable> (x: 3)    float64            [rad]  [0, 0.523599, 1.5708]

    The output is in radians:

      >>> sc.asin(sc.scalar(0.5)).to(unit='deg')
      <scipp.Variable> ()    float64            [deg]  30
    """
    return _call_cpp_func(_cpp.asin, x, out=out)  # type: ignore[return-value]


def acos(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
    """Element-wise inverse cosine.

    The input must be dimensionless and in the range [-1, 1].

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

    Examples
    --------
    Compute the inverse cosine:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 0.5, 0.0])
      >>> sc.acos(x)
      <scipp.Variable> (x: 3)    float64            [rad]  [0, 1.0472, 1.5708]

    The output is in radians, which can be converted to degrees:

      >>> sc.acos(sc.scalar(0.5)).to(unit='deg')
      <scipp.Variable> ()    float64            [deg]  60
    """
    return _call_cpp_func(_cpp.acos, x, out=out)  # type: ignore[return-value]


def atan(
    x: VariableLikeType, *, out: VariableLikeType | None = None
) -> VariableLikeType:
    """Element-wise inverse tangent.

    The input must be dimensionless.

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

    See Also
    --------
    scipp.atan2:
        Two-argument inverse tangent for determining the correct quadrant.

    Examples
    --------
    Compute the inverse tangent:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[0.0, 1.0, -1.0])
      >>> sc.atan(x)
      <scipp.Variable> (x: 3)    float64            [rad]  [0, 0.785398, -0.785398]

    The output is in radians in the range [-π/2, π/2]:

      >>> sc.atan(sc.scalar(1.0)).to(unit='deg')
      <scipp.Variable> ()    float64            [deg]  45
    """
    return _call_cpp_func(_cpp.atan, x, out=out)  # type: ignore[return-value]


def atan2(*, y: Variable, x: Variable, out: Variable | None = None) -> Variable:
    """Element-wise inverse tangent of y/x determining the correct quadrant.

    Unlike ``atan(y/x)``, this function uses the signs of both arguments
    to determine the correct quadrant of the result.

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

    Examples
    --------
    Compute the angle in radians between the positive x-axis and the point (x, y):

      >>> import scipp as sc
      >>> y = sc.array(dims=['point'], values=[0.0, 1.0, 1.0], unit='m')
      >>> x = sc.array(dims=['point'], values=[1.0, 0.0, 1.0], unit='m')
      >>> sc.atan2(y=y, x=x)
      <scipp.Variable> (point: 3)    float64            [rad]  [0, 1.5708, 0.785398]

    The x and y arguments must have the same unit. The output is in radians:

      >>> y = sc.array(dims=['point'], values=[1.0, -1.0], unit='m')
      >>> x = sc.array(dims=['point'], values=[-1.0, -1.0], unit='m')
      >>> sc.atan2(y=y, x=x).to(unit='deg')
      <scipp.Variable> (point: 2)    float64            [deg]  [135, -135]
    """
    return _call_cpp_func(_cpp.atan2, y=y, x=x, out=out)  # type: ignore[return-value]
