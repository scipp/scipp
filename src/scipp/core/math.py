# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations

from typing import TypeVar, overload

from .._scipp import core as _cpp
from ..typing import VariableLike, VariableLikeType
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .cpp_classes import Unit, Variable
from .variable import scalar

_T = TypeVar("_T")


@overload
def abs(x: Variable, *, out: Variable | None = None) -> Variable: ...


@overload
def abs(x: VariableLikeType) -> VariableLikeType: ...


@overload
def abs(x: Unit) -> Unit: ...


def abs(x: _T, *, out: Variable | None = None) -> _T:
    """Element-wise absolute value.

    Parameters
    ----------
    x:
        Input data.
    out:
        Optional output buffer. Only supported if `x` is a scipp.Variable.

    Raises
    ------
    scipp.DTypeError
        If the dtype has no absolute value, e.g., if it is a string.

    Returns
    -------
    :
        The absolute values of the input.

    See Also
    --------
    scipp.norm

    Examples
    --------
    Basic usage with negative values:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[-2.0, -1.0, 0.0, 1.0, 2.0], unit='m')
      >>> sc.abs(x)
      <scipp.Variable> (x: 5)    float64              [m]  [2, 1, ..., 1, 2]

    Units are preserved in the output:

      >>> x = sc.array(dims=['x'], values=[-5.0, -3.0, 3.0], unit='kg')
      >>> sc.abs(x)
      <scipp.Variable> (x: 3)    float64             [kg]  [5, 3, 3]
    """
    return _call_cpp_func(_cpp.abs, x, out=out)  # type: ignore[return-value]


def cross(x: VariableLikeType, y: VariableLikeType) -> VariableLikeType:
    """Element-wise cross product.

    Parameters
    ----------
    x:
        Left hand side operand.
    y:
        Right hand side operand.

    Raises
    ------
    scipp.DTypeError
        If the dtype of the input is not vector3.

    Returns
    -------
    :
        The cross product of the input vectors.

    Examples
    --------
    Compute cross product of 3D vectors:

      >>> import scipp as sc
      >>> v1 = sc.vectors(dims=['x'], values=[[1.0, 0.0, 0.0], [0.0, 1.0, 0.0]], unit='m')
      >>> v2 = sc.vectors(dims=['x'], values=[[0.0, 1.0, 0.0], [0.0, 0.0, 1.0]], unit='m')
      >>> sc.cross(v1, v2)
      <scipp.Variable> (x: 2)    vector3            [m^2]  [(0, 0, 1), (1, 0, 0)]

    The cross product of two vectors is perpendicular to both input vectors.
    Units are multiplied in the result.
    """  # noqa: E501
    return _call_cpp_func(_cpp.cross, x, y)  # type: ignore[return-value]


def dot(x: VariableLikeType, y: VariableLikeType) -> VariableLikeType:
    """Element-wise dot product.

    Parameters
    ----------
    x:
        Left hand side operand.
    y:
        Right hand side operand.

    Raises
    ------
    scipp.DTypeError
        If the dtype of the input is not vector3.

    Returns
    -------
    :
        The dot product of the input vectors.

    Examples
    --------
    Compute dot product of 3D vectors:

      >>> import scipp as sc
      >>> v1 = sc.vectors(dims=['x'], values=[[1.0, 0.0, 0.0], [1.0, 1.0, 0.0]], unit='m')
      >>> v2 = sc.vectors(dims=['x'], values=[[1.0, 0.0, 0.0], [0.0, 1.0, 0.0]], unit='m')
      >>> sc.dot(v1, v2)
      <scipp.Variable> (x: 2)    float64            [m^2]  [1, 1]

    The result is a scalar for each pair of vectors.
    Units are multiplied in the result.
    """  # noqa: E501
    return _call_cpp_func(_cpp.dot, x, y)  # type: ignore[return-value]


def nan_to_num(
    x: Variable,
    *,
    nan: Variable | None = None,
    posinf: Variable | None = None,
    neginf: Variable | None = None,
    out: Variable | None = None,
) -> Variable:
    """Element-wise special value replacement.

    All elements in the output are identical to input except in the presence
    of a NaN, Inf or -Inf.
    The function allows replacements to be separately specified for NaN, Inf
    or -Inf values.
    You can choose to replace a subset of those special values by providing
    just the required keyword arguments.

    Parameters
    ----------
    x:
        Input data.
    nan:
        Replacement values for NaN in the input.
    posinf:
        Replacement values for Inf in the input.
    neginf:
        Replacement values for -Inf in the input.
    out:
        Optional output buffer.

    Raises
    ------
    scipp.DTypeError
        If the types of input and replacement do not match.

    Returns
    -------
    :
        Input with specified substitutions.

    Examples
    --------
    Replace special values with custom values:

      >>> import scipp as sc
      >>> import numpy as np
      >>> x = sc.array(dims=['x'], values=[1.0, np.nan, np.inf, -np.inf, 2.0])
      >>> sc.nan_to_num(x, nan=sc.scalar(0.0), posinf=sc.scalar(999.0), neginf=sc.scalar(-999.0))
      <scipp.Variable> (x: 5)    float64  [dimensionless]  [1, 0, ..., -999, 2]

    Replace only NaN values, leaving infinities unchanged:

      >>> y = sc.array(dims=['x'], values=[1.0, np.nan, 2.0])
      >>> sc.nan_to_num(y, nan=sc.scalar(-1.0))
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [1, -1, 2]
    """  # noqa: E501
    return _call_cpp_func(  # type: ignore[return-value]
        _cpp.nan_to_num, x, nan=nan, posinf=posinf, neginf=neginf, out=out
    )


def norm(x: VariableLikeType) -> VariableLikeType:
    """Element-wise norm.

    Parameters
    ----------
    x:
        Input data.

    Raises
    ------
    scipp.DTypeError
        If the dtype has no norm, i.e., if it is not a vector.

    Returns
    -------
    :
        Scalar elements computed as the norm values of the input elements.
    """
    return _call_cpp_func(_cpp.norm, x, out=None)  # type: ignore[return-value]


@overload
def reciprocal(x: Variable, *, out: Variable | None = None) -> Variable: ...


@overload
def reciprocal(x: VariableLikeType) -> VariableLikeType: ...


@overload
def reciprocal(x: Unit) -> Unit: ...


def reciprocal(x: _T, *, out: Variable | None = None) -> _T:
    """Element-wise reciprocal.

    Parameters
    ----------
    x:
        Input data.
    out:
        Optional output buffer. Only supported when `x` is a scipp.Variable.

    Raises
    ------
    scipp.DTypeError
        If the dtype has no reciprocal, e.g., if it is a string.

    Returns
    -------
    :
        The reciprocal values of the input.

    Examples
    --------
    Compute reciprocal (1/x):

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 4.0], unit='m')
      >>> sc.reciprocal(x)
      <scipp.Variable> (x: 3)    float64            [1/m]  [1, 0.5, 0.25]

    Units are inverted in the result:

      >>> x = sc.array(dims=['x'], values=[2.0, 5.0], unit='s')
      >>> sc.reciprocal(x)
      <scipp.Variable> (x: 2)    float64             [Hz]  [0.5, 0.2]
    """
    return _call_cpp_func(_cpp.reciprocal, x, out=out)  # type: ignore[return-value]


@overload
def pow(
    base: VariableLikeType, exponent: VariableLikeType | float
) -> VariableLikeType: ...


@overload
def pow(base: Unit, exponent: Variable | float) -> Unit: ...


def pow(base: _T, exponent: VariableLike | float) -> _T:
    """Element-wise power.

    If the base has a unit, the exponent must be scalar in order to get
    a well-defined unit in the result.

    Parameters
    ----------
    base:
        Base of the exponential.
    exponent:
        Raise ``base`` to this power.

    Raises
    ------
    scipp.DTypeError
        If the dtype does not have a power, e.g., if it is a string.

    Returns
    -------
    :
        ``base`` raised to the power of ``exp``.

    Examples
    --------
    Raise values to a scalar power:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> sc.pow(x, 2)
      <scipp.Variable> (x: 3)    float64            [m^2]  [1, 4, 9]

    Element-wise power with array exponents (base must be dimensionless):

      >>> x = sc.array(dims=['x'], values=[2.0, 4.0, 8.0])
      >>> y = sc.array(dims=['x'], values=[1.0, 2.0, 3.0])
      >>> sc.pow(x, y)
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [2, 16, 512]

    Units are raised to the power in the result.
    """
    if not isinstance(base, _cpp.Unit) and isinstance(exponent, float | int):
        exponent = scalar(exponent)
    return _call_cpp_func(_cpp.pow, base, exponent)  # type: ignore[return-value]


@overload
def sqrt(x: Variable, *, out: Variable | None = None) -> Variable: ...


@overload
def sqrt(x: VariableLikeType) -> VariableLikeType: ...


@overload
def sqrt(x: Unit) -> Unit: ...


def sqrt(x: _T, *, out: Variable | None = None) -> _T:
    """Element-wise square-root.

    Parameters
    ----------
    x:
        Input data.
    out:
        Optional output buffer. Only supported when `x` is a scipp.Variable.

    Raises
    ------
    scipp.DTypeError
         If the dtype has no square-root, e.g., if it is a string.

    Returns
    -------
    :
        The square-root values of the input.
    """
    return _call_cpp_func(_cpp.sqrt, x, out=out)  # type: ignore[return-value]


@overload
def exp(x: Variable, *, out: Variable | None = None) -> Variable: ...


@overload
def exp(x: VariableLikeType) -> VariableLikeType: ...


def exp(x: _T, *, out: Variable | None = None) -> _T:
    """Element-wise exponential.

    Parameters
    ----------
    x:
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    :
        e raised to the power of the input.

    Examples
    --------
    Compute exponential function:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[0.0, 1.0, 2.0])
      >>> sc.exp(x)
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [1, 2.71828, 7.38906]

    The input must be dimensionless.
    """
    return _call_cpp_func(_cpp.exp, x, out=out)  # type: ignore[return-value]


@overload
def log(x: Variable, *, out: Variable | None = None) -> Variable: ...


@overload
def log(x: VariableLikeType) -> VariableLikeType: ...


def log(x: _T, *, out: Variable | None = None) -> _T:
    """Element-wise natural logarithm.

    Parameters
    ----------
    x:
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    :
        Base e logarithm of the input.

    Examples
    --------
    Compute natural logarithm:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.718, 7.389])
      >>> sc.log(x)
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [0, 0.999896, 1.99999]

    The input must be dimensionless and positive.
    """
    return _call_cpp_func(_cpp.log, x, out=out)  # type: ignore[return-value]


@overload
def log10(x: Variable, *, out: Variable | None = None) -> Variable: ...


@overload
def log10(x: VariableLikeType) -> VariableLikeType: ...


def log10(x: _T, *, out: Variable | None = None) -> _T:
    """Element-wise base 10 logarithm.

    Parameters
    ----------
    x:
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    :
        Base 10 logarithm of the input.

    Examples
    --------
    Compute base 10 logarithm:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 10.0, 100.0, 1000.0])
      >>> sc.log10(x)
      <scipp.Variable> (x: 4)    float64  [dimensionless]  [0, 1, 2, 3]

    The input must be dimensionless and positive.
    """
    return _call_cpp_func(_cpp.log10, x, out=out)  # type: ignore[return-value]


@overload
def round(
    x: Variable, *, decimals: int = 0, out: Variable | None = None
) -> Variable: ...


@overload
def round(x: VariableLikeType, *, decimals: int = 0) -> VariableLikeType: ...


def round(x: _T, *, decimals: int = 0, out: Variable | None = None) -> _T:
    """
    Round to the given number of decimals.

    Note: if the number being rounded is halfway between two numbers it will
    round to the nearest even number. For example 1.5 and 2.5 will both round
    to 2.0, -0.5 and 0.5 will both round to 0.0.

    For non-zero ``decimals``, the implementation uses
    ``rint(x * 10**decimals) / 10**decimals``, which can overflow for very
    large values of ``x`` combined with large positive ``decimals``.
    This is the same algorithm and limitation as :py:func:`numpy.round`.

    Parameters
    ----------
    x:
        Input data.
    decimals:
        Number of decimal places to round to (default: 0).
        If decimals is negative, it specifies the number of positions
        to the left of the decimal point.
    out:
        Optional output buffer.

    Returns
    -------
    :
        Rounded version of the data passed.

    See Also
    --------
    scipp.ceil, scipp.floor

    Examples
    --------
    Round to the nearest integer:

      >>> import scipp as sc
      >>> sc.round(sc.scalar(1.5))
      <scipp.Variable> ()    float64  [dimensionless]  2
      >>> sc.round(sc.scalar(1.567), decimals=2)
      <scipp.Variable> ()    float64  [dimensionless]  1.57
      >>> sc.round(sc.scalar(12345.0), decimals=-2)
      <scipp.Variable> ()    float64  [dimensionless]  12300
    """
    if decimals == 0:
        return _call_cpp_func(_cpp.rint, x, out=out)  # type: ignore[return-value]
    factor = 10**decimals
    result = _call_cpp_func(_cpp.rint, factor * x) / factor
    if out is not None:
        out[...] = result
        return out  # type: ignore[return-value]
    return result  # type: ignore[no-any-return]


@overload
def floor(x: Variable, *, out: Variable | None = None) -> Variable: ...


@overload
def floor(x: VariableLikeType) -> VariableLikeType: ...


def floor(x: _T, *, out: Variable | None = None) -> _T:
    """
    Round down to the nearest integer of all values passed in x.

    Parameters
    ----------
    x:
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    :
        Rounded down version of the data passed.
    """
    return _call_cpp_func(_cpp.floor, x, out=out)  # type: ignore[return-value]


@overload
def ceil(x: Variable, *, out: Variable | None = None) -> Variable: ...


@overload
def ceil(x: VariableLikeType) -> VariableLikeType: ...


def ceil(x: _T, *, out: Variable | None = None) -> _T:
    """
    Round up to the nearest integer of all values passed in x.

    Parameters
    ----------
    x:
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    :
        Rounded up version of the data passed.
    """
    return _call_cpp_func(_cpp.ceil, x, out=out)  # type: ignore[return-value]


def erf(x: VariableLikeType) -> VariableLikeType:
    """
    Computes the error function.

    Parameters
    ----------
    x:
        Input data.

    Returns
    -------
    :
        Error function values of the input.

    Examples
    --------
    Compute error function:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[-2.0, -1.0, 0.0, 1.0, 2.0])
      >>> sc.erf(x)
      <scipp.Variable> (x: 5)    float64  [dimensionless]  [-0.995322, -0.842701, ..., 0.842701, 0.995322]

    The error function is odd symmetric: erf(-x) = -erf(x).
    """  # noqa: E501
    return _call_cpp_func(_cpp.erf, x)  # type: ignore[return-value]


def erfc(x: VariableLikeType) -> VariableLikeType:
    """
    Computes the complementary error function.

    Parameters
    ----------
    x:
        Input data.

    Returns
    -------
    :
        Complementary error function values of the input.

    Examples
    --------
    Compute complementary error function:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[-2.0, -1.0, 0.0, 1.0, 2.0])
      >>> sc.erfc(x)
      <scipp.Variable> (x: 5)    float64  [dimensionless]  [1.99532, 1.8427, ..., 0.157299, 0.00467773]

    The complementary error function is defined as erfc(x) = 1 - erf(x).
    """  # noqa: E501
    return _call_cpp_func(_cpp.erfc, x)  # type: ignore[return-value]


def midpoints(x: Variable, dim: str | None = None) -> Variable:
    """
    Computes the points in the middle of adjacent elements of x.

    The midpoint of two numbers :math:`a` and :math:`b` is
    :math:`(a + b) / 2`.
    This formula encounters under- or overflow for
    very small or very large inputs.
    The implementation deals with those cases properly.

    Parameters
    ----------
    x:
        Input data.
    dim:
        Dimension along which to compute midpoints.
        Optional for 1D Variables.

    Returns
    -------
    :
        Midpoints of ``x`` along ``dim``.

    Examples
    --------
    Compute midpoints along a dimension:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[-2, 0, 4, 2])
      >>> x
      <scipp.Variable> (x: 4)      int64  [dimensionless]  [-2, 0, 4, 2]
      >>> sc.midpoints(x)
      <scipp.Variable> (x: 3)      float64  [dimensionless]  [-1, 2, 3]

    `midpoints` casts integers to float before computing the midpoints:

      >>> x = sc.array(dims=['x'], values=[0, 3, 0])
      >>> x
      <scipp.Variable> (x: 3)      int64  [dimensionless]  [0, 3, 0]
      >>> sc.midpoints(x)
      <scipp.Variable> (x: 2)      float64  [dimensionless]  [1.5, 1.5]

    With multidimensional variables, `midpoints` is only applied
    to the specified dimension:

      >>> xy = sc.array(dims=['x', 'y'], values=[[1, 3, 5], [2, 6, 10]])
      >>> xy.values
      array([[ 1,  3,  5],
             [ 2,  6, 10]])
      >>> sc.midpoints(xy, dim='x').values
      array([[1.5, 4.5, 7.5]])
      >>> sc.midpoints(xy, dim='y').values
      array([[2., 4.],
             [4., 8.]])
    """
    return _cpp.midpoints(x, dim)  # type: ignore[no-any-return]
