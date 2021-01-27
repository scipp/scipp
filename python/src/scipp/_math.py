# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def abs(x, out=None):
    """Element-wise absolute value.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the dtype has no absolute value, e.g., if it is a string.
    :return: The absolute values of the input.
    :seealso: :py:func:`scipp.norm` for vector-like dtype.
    """
    return _call_cpp_func(_cpp.abs, x, out=out)


def nan_to_num(x, nan=None, posinf=None, neginf=None, out=None):
    """Element-wise special value replacement.

    All elements in the output are identical to input except in the presence
    of a NaN, Inf or -Inf.
    The function allows replacements to be separately specified for nan, inf
    or -inf values.
    You can choose to replace a subset of those special values by providing
    just the required keyword arguments.
    If the replacement is value-only and the input has variances, the variance
    at the element(s) undergoing replacement are also replaced with the
    replacement value.
    If the replacement has a variance and the input has variances, the
    variance at the element(s) undergoing replacement are also replaced with
    the replacement variance.

    :param x: Input data.
    :param nan: Replacement values for NaN in the input.
    :param posinf: Replacement values for Inf in the input.
    :param neginf: Replacement values for -Inf in the input.
    :param out: Optional output buffer.
    :raises: If the types of input and replacement do not match.
    :return: Input elements are replaced in output with specified subsitutions.
    """
    return _call_cpp_func(_cpp.nan_to_num, x, nan, posinf, neginf, out=out)


def norm(x):
    """Element-wise norm.

    :param x: Input data.
    :raises: If the dtype has no norm, i.e., if it is not a vector.
    :return: Scalar elements computed as the norm values of the input elements.
    """
    return _call_cpp_func(_cpp.norm, x, out=None)


def reciprocal(x, out=None):
    """Element-wise reciprocal.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the dtype has no reciprocal, e.g., if it is a string.
    :return: The reciprocal values of the input.
    """
    return _call_cpp_func(_cpp.reciprocal, x, out=out)


def sqrt(x, out=None):
    """Element-wise square-root.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the dtype has no square-root, e.g., if it is a string.
    :return: The square-root values of the input.
    """
    return _call_cpp_func(_cpp.sqrt, x, out=out)


def exp(x):
    """Element-wise exponentiation.

    :param x: Input data.
    :returns: e raised to the power of the input.
    """
    return _call_cpp_func(_cpp.exp, x)


def log(x):
    """Element-wise natural logarithm.

    :param x: Input data.
    :returns: Base e logiarithm of the input.
    """
    return _call_cpp_func(_cpp.log, x)


def log10(x):
    """Element-wise base 10 logarithm.

    :param x: Input data.
    :returns: Base 10 logarithm of the input.
    """
    return _call_cpp_func(_cpp.log10, x)
