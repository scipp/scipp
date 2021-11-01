# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from __future__ import annotations
from typing import Optional

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLike


def sin(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise sine.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not a plane-angle unit, or if the sine cannot be
             computed on the dtype, e.g., if it is an integer.
    :return: The sine values of the input.
    """
    return _call_cpp_func(_cpp.sin, x, out=out)


def cos(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise cosine.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not a plane-angle unit, or if the cosine cannot be
             computed on the dtype, e.g., if it is an integer.
    :return: The cosine values of the input.
    """
    return _call_cpp_func(_cpp.cos, x, out=out)


def tan(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise tangent.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not a plane-angle unit, or if the tangent cannot be
             computed on the dtype, e.g., if it is an integer.
    :return: The tangent values of the input.
    """
    return _call_cpp_func(_cpp.tan, x, out=out)


def asin(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise inverse sine.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not dimensionless.
    :return: The inverse sine values of the input.
    """
    return _call_cpp_func(_cpp.asin, x, out=out)


def acos(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise inverse cosine.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not dimensionless.
    :return: The inverse cosine values of the input.
    """
    return _call_cpp_func(_cpp.acos, x, out=out)


def atan(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise inverse tangent.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not dimensionless.
    :return: The inverse tangent values of the input.
    """
    return _call_cpp_func(_cpp.atan, x, out=out)


def atan2(*,
          y: _cpp.Variable,
          x: _cpp.Variable,
          out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Element-wise inverse tangent of y/x determining the correct quadrant.

    :param y: Input y values.
    :param x: Input x values.
    :param out: Optional output buffer.
    :return: The signed inverse tangent values of y/x in the range [-π, π].
    :seealso: All edge cases are documented
              `here <https://en.cppreference.com/w/c/numeric/math/atan2>`_.
              Note that domain errors are *not* propagated to Python.
    """
    return _call_cpp_func(_cpp.atan2, y=y, x=x, out=out)
