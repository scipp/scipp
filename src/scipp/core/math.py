# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations
from typing import Optional, Union
from numbers import Real

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLike
from .variable import scalar


def abs(x: Union[VariableLike, _cpp.Unit],
        *,
        out: Optional[_cpp.Variable] = None) -> Union[VariableLike, _cpp.Unit]:
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
    """
    return _call_cpp_func(_cpp.abs, x, out=out)


def cross(x: VariableLike, y: VariableLike) -> VariableLike:
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
    """
    return _call_cpp_func(_cpp.cross, x, y)


def dot(x: VariableLike, y: VariableLike) -> VariableLike:
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
    """
    return _call_cpp_func(_cpp.dot, x, y)


def nan_to_num(x: _cpp.Variable,
               *,
               nan: _cpp.Variable = None,
               posinf: _cpp.Variable = None,
               neginf: _cpp.Variable = None,
               out: _cpp.Variable = None) -> _cpp.Variable:
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
    """
    return _call_cpp_func(_cpp.nan_to_num,
                          x,
                          nan=nan,
                          posinf=posinf,
                          neginf=neginf,
                          out=out)


def norm(x: VariableLike) -> VariableLike:
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
    return _call_cpp_func(_cpp.norm, x, out=None)


def reciprocal(x: Union[VariableLike, _cpp.Unit],
               *,
               out: Optional[_cpp.Variable] = None) -> Union[VariableLike, _cpp.Unit]:
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
    """
    return _call_cpp_func(_cpp.reciprocal, x, out=out)


def pow(base: Union[VariableLike, _cpp.Unit],
        exponent: Union[VariableLike, Real]) -> Union[VariableLike, _cpp.Unit]:
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
    """
    if not isinstance(base, _cpp.Unit) and isinstance(exponent, Real):
        exponent = scalar(exponent)
    return _call_cpp_func(_cpp.pow, base, exponent)


def sqrt(x: Union[VariableLike, _cpp.Unit],
         *,
         out: Optional[_cpp.Variable] = None) -> Union[VariableLike, _cpp.Unit]:
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
    return _call_cpp_func(_cpp.sqrt, x, out=out)


def exp(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
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
    """
    return _call_cpp_func(_cpp.exp, x, out=out)


def log(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
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
    """
    return _call_cpp_func(_cpp.log, x, out=out)


def log10(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
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
    """
    return _call_cpp_func(_cpp.log10, x, out=out)


def round(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """
    Round to the nearest integer of all values passed in x.

    Note: if the number being rounded is halfway between two integers it will
    round to the nearest even number. For example 1.5 and 2.5 will both round
    to 2.0, -0.5 and 0.5 will both round to 0.0.

    Parameters
    ----------
    x:
        Input data.
    out:
        Optional output buffer.

    Returns
    -------
    :
        Rounded version of the data passed to the nearest integer.
    """
    return _call_cpp_func(_cpp.rint, x, out=out)


def floor(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
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
    return _call_cpp_func(_cpp.floor, x, out=out)


def ceil(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
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
    return _call_cpp_func(_cpp.ceil, x, out=out)


def erf(x: VariableLike) -> VariableLike:
    """
    Computes the error function.

    Parameters
    ----------
    x:
        Input data.
    """
    return _cpp.erf(x)


def erfc(x: VariableLike) -> VariableLike:
    """
    Computes the complementary error function.

    Parameters
    ----------
    x:
        Input data.
    """
    return _cpp.erfc(x)


def midpoints(x: _cpp.Variable, dim: Optional[str] = None) -> _cpp.Variable:
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

      >>> x = sc.array(dims=['x'], values=[-2, 0, 4, 2])
      >>> x
      <scipp.Variable> (x: 4)      int64  [dimensionless]  [-2, 0, 4, 2]
      >>> sc.midpoints(x)
      <scipp.Variable> (x: 3)      int64  [dimensionless]  [-1, 2, 3]

    For integers, when the difference of adjacent elements is odd,
    `midpoints` rounds towards the number that comes first in the array:

      >>> x = sc.array(dims=['x'], values=[0, 3, 0])
      >>> x
      <scipp.Variable> (x: 3)      int64  [dimensionless]  [0, 3, 0]
      >>> sc.midpoints(x)
      <scipp.Variable> (x: 2)      int64  [dimensionless]  [1, 2]

    With multidimensional variables, `midpoints` is only applied
    to the specified dimension:

      >>> xy = sc.array(dims=['x', 'y'], values=[[1, 3, 5], [2, 6, 10]])
      >>> xy.values
      array([[ 1,  3,  5],
             [ 2,  6, 10]])
      >>> sc.midpoints(xy, dim='x').values
      array([[1, 4, 7]])
      >>> sc.midpoints(xy, dim='y').values
      array([[2, 4],
             [4, 8]])
    """
    return _cpp.midpoints(x, dim)
