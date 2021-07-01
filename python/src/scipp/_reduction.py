# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from __future__ import annotations
from typing import Optional

from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .typing import LabeledArray


def mean(x: LabeledArray,
         dim: Optional[str] = None,
         out: Optional[LabeledArray] = None) -> LabeledArray:
    """Element-wise mean over the specified dimension.

    If the input has variances, the variances stored in the output are based on
    the "standard deviation of the mean", i.e.,
    :math:`\\sigma_{mean} = \\sigma / \\sqrt{N}`.
    :math:`N` is the length of the input dimension.
    :math:`\\sigma` is estimated as the average of the standard deviations of
    the input elements along that dimension.

    :param x: Input data.
    :param dim: Dimension along which to calculate the mean. If not
                given, the mean over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The mean of the input values.
    :seealso: :py:func:`scipp.nanmean`.
    """
    if dim is None:
        return _call_cpp_func(_cpp.mean, x, out=out)
    else:
        return _call_cpp_func(_cpp.mean, x, dim=dim, out=out)


def nanmean(x: LabeledArray,
            dim: Optional[str] = None,
            out: Optional[LabeledArray] = None) -> LabeledArray:
    """Element-wise mean over the specified dimension ignoring NaNs.

    If the input has variances, the variances stored in the ouput are based on
    the "standard deviation of the mean", i.e.,
    :math:`\\sigma_{mean} = \\sigma / \\sqrt{N}`.
    :math:`N` is the length of the input dimension.
    :math:`\\sigma` is estimated as the average of the standard deviations of
    the input elements along that dimension.

    :param x: Input data.
    :param dim: Dimension along which to calculate the mean. If not
                given, the nanmean over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The mean of the input values.
    :seealso: :py:func:`scipp.mean`.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nanmean, x, out=out)
    else:
        return _call_cpp_func(_cpp.nanmean, x, dim=dim, out=out)


def sum(x: LabeledArray,
        dim: Optional[str] = None,
        out: Optional[LabeledArray] = None) -> LabeledArray:
    """Element-wise sum over the specified dimension.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the sum. If not
                given, the sum over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The sum of the input values.
    :seealso: :py:func:`scipp.nansum`.
    """
    if dim is None:
        return _call_cpp_func(_cpp.sum, x, out=out)
    else:
        return _call_cpp_func(_cpp.sum, x, dim=dim, out=out)


def nansum(x: LabeledArray,
           dim: Optional[str] = None,
           out: Optional[LabeledArray] = None) -> LabeledArray:
    """Element-wise sum over the specified dimension; NaNs are treated as zero.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the sum. If not
                given, the sum over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The sum of the input values with NaNs set to zero.
    :seealso: :py:func:`scipp.sum`.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nansum, x, out=out)
    else:
        return _call_cpp_func(_cpp.nansum, x, dim=dim, out=out)


def min(x: _cpp.Variable,
        dim: Optional[str] = None,
        out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Element-wise min over the specified dimension or all dimensions if not
    provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the min. If not
                given, the min over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The min of the input values.
    :seealso: :py:func:`scipp.nanmin`, :py:func:`scipp.nanmax`.
    """
    if dim is None:
        return _call_cpp_func(_cpp.min, x, out=out)
    else:
        return _call_cpp_func(_cpp.min, x, dim=dim, out=out)


def max(x: _cpp.Variable,
        dim: Optional[str] = None,
        out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Element-wise max over the specified dimension or all dimensions if not
    provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the max. If not
                given, the max over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The max of the input values.
    :seealso: :py:func:`scipp.nanmax`, :py:func:`scipp.min`.
    """
    if dim is None:
        return _call_cpp_func(_cpp.max, x, out=out)
    else:
        return _call_cpp_func(_cpp.max, x, dim=dim, out=out)


def nanmin(x: _cpp.Variable,
           dim: Optional[str] = None,
           out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Element-wise min ignoring not at number values over the specified
    dimension or all dimensions if not provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the min. If not
                given, the min over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The min of the input values.
    :seealso: :py:func:`scipp.min`, :py:func:`scipp.nanmax`.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nanmin, x, out=out)
    else:
        return _call_cpp_func(_cpp.nanmin, x, dim=dim, out=out)


def nanmax(x: _cpp.Variable,
           dim: Optional[str] = None,
           out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Element-wise max ignoring not a number values over the specified
    dimension or all dimensions if not provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the max. If not
                given, the max over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The max of the input values.
    :seealso: :py:func:`scipp.max`, :py:func:`scipp.nanmin`.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nanmax, x, out=out)
    else:
        return _call_cpp_func(_cpp.nanmax, x, dim=dim, out=out)


def all(x: _cpp.Variable,
        dim: Optional[str] = None,
        out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Element-wise AND over the specified dimension or all dimensions if not
    provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the AND. If not
                given, the AND over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The AND of the input values.
    :seealso: :py:func:`scipp.any`.
    """
    if dim is None:
        return _call_cpp_func(_cpp.all, x, out=out)
    else:
        return _call_cpp_func(_cpp.all, x, dim=dim, out=out)


def any(x: _cpp.Variable,
        dim: Optional[str] = None,
        out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Element-wise OR over the specified dimension or all dimensions if not
    provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the OR. If not
                given, the OR over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The OR of the input values.
    :seealso: :py:func:`scipp.all`.
    """
    if dim is None:
        return _call_cpp_func(_cpp.any, x, out=out)
    else:
        return _call_cpp_func(_cpp.any, x, dim=dim, out=out)
