# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations
from typing import Optional

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLike


def abs(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise absolute value.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the dtype has no absolute value, e.g., if it is a string.
    :return: The absolute values of the input.
    :seealso: :py:func:`scipp.norm` for vector-like dtype.
    """
    return _call_cpp_func(_cpp.abs, x, out=out)


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

    :param x: Input data.
    :param nan: Replacement values for NaN in the input.
    :param posinf: Replacement values for Inf in the input.
    :param neginf: Replacement values for -Inf in the input.
    :param out: Optional output buffer.
    :raises: If the types of input and replacement do not match.
    :return: Input with specified substitutions.
    """
    return _call_cpp_func(_cpp.nan_to_num,
                          x,
                          nan=nan,
                          posinf=posinf,
                          neginf=neginf,
                          out=out)


def norm(x: VariableLike) -> VariableLike:
    """Element-wise norm.

    :param x: Input data.
    :raises: If the dtype has no norm, i.e., if it is not a vector.
    :return: Scalar elements computed as the norm values of the input elements.
    """
    return _call_cpp_func(_cpp.norm, x, out=None)


def reciprocal(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise reciprocal.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the dtype has no reciprocal, e.g., if it is a string.
    :return: The reciprocal values of the input.
    """
    return _call_cpp_func(_cpp.reciprocal, x, out=out)


def pow(base: VariableLike, exp: VariableLike) -> VariableLike:
    """Element-wise power.

    If the base has a unit, the exponent must be scalar in order to get
    a well defined unit in the result.

    :raises: If the dtype does not have a power, e.g., if it is a string.
    :return: ``base`` raised to the power of ``exp``.
    """
    return _call_cpp_func(_cpp.pow, base, exp)


def sqrt(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise square-root.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the dtype has no square-root, e.g., if it is a string.
    :return: The square-root values of the input.
    """
    return _call_cpp_func(_cpp.sqrt, x, out=out)


def exp(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise exponential.

    :param x: Input data.
    :param out: Optional output buffer.
    :returns: e raised to the power of the input.
    """
    return _call_cpp_func(_cpp.exp, x, out=out)


def log(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise natural logarithm.

    :param x: Input data.
    :param out: Optional output buffer.
    :returns: Base e logiarithm of the input.
    """
    return _call_cpp_func(_cpp.log, x, out=out)


def log10(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """Element-wise base 10 logarithm.

    :param x: Input data.
    :param out: Optional output buffer.
    :returns: Base 10 logarithm of the input.
    """
    return _call_cpp_func(_cpp.log10, x, out=out)


def round(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """
    Round to the nearest integer if all values passed in x.

    Note: if the number being rounded is halfway between two integers it will
    round to the nearest even number. For example 1.5 and 2.5 will both round
    to 2.0, -0.5 and 0.5 will both round to 0.0.

    :param x: Input data.
    :param out: Optional output buffer.
    :returns: Rounded version of the data passed to the nearest integer.
    """
    return _call_cpp_func(_cpp.rint, x, out=out)


def floor(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """
    Round down to the nearest integer of all values passed in x.

    :param x: Input data.
    :param out: Optional output buffer.
    :returns: Rounded down version of the data passed.
    """
    return _call_cpp_func(_cpp.floor, x, out=out)


def ceil(x: VariableLike, *, out: Optional[VariableLike] = None) -> VariableLike:
    """
    Round up to the nearest integer of all values passed in x.

    :param x: Input data.
    :param out: Optional output buffer.
    :returns: Rounded up version of the data passed.
    """
    return _call_cpp_func(_cpp.ceil, x, out=out)


def erf(x: VariableLike) -> VariableLike:
    """
    Computes the error function.
    """
    return _cpp.erf(x)


def erfc(x: VariableLike) -> VariableLike:
    """
    Computes the complementary error function.
    """
    return _cpp.erfc(x)
