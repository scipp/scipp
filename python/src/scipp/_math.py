# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from __future__ import annotations
from typing import Optional

from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .typing import VariableLike


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
