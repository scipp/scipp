# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def mean(x, dim=None, out=None):
    """Element-wise mean over the specified dimension, if variances are
    present, the new variance is computed as standard-deviation of the mean.

    If the input has variances, the variances stored in the ouput are based on
    the "standard deviation of the mean", i.e.,
    :math:`\\sigma_{mean} = \\sigma / \\sqrt{N}`.
    :math:`N` is the length of the input dimension.
    :math:`sigma` is estimated as the average of the standard deviations of
    the input elements along that dimension.

    :param x: Input data.
    :param dim: Dimension along which to calculate the mean. If not
                given, the mean over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The mean of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.mean, x, out=out)
    else:
        return _call_cpp_func(_cpp.mean, x, dim=dim, out=out)


def nanmean(x, dim=None, out=None):
    """Element-wise mean over the specified dimension, if variances are
    present, the new variance is computed as standard-deviation of the mean.
    NaNs are ignored.
    This function only supports variables of floating point types as input.

    If the input has variances, the variances stored in the ouput are based on
    the "standard deviation of the mean", i.e.,
    :math:`\\sigma_{mean} = \\sigma / \\sqrt{N}`.
    :math:`N` is the length of the input dimension.
    :math:`sigma` is estimated as the average of the standard deviations of
    the input elements along that dimension.

    :param x: Input data.
    :param dim: Dimension along which to calculate the mean. If not
                given, the nanmean over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The mean of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nanmean, x, out=out)
    else:
        return _call_cpp_func(_cpp.nanmean, x, dim=dim, out=out)


def sum(x, dim=None, out=None):
    """Element-wise sum over the specified dimension.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the sum. If not
                given, the sum over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The sum of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.sum, x, out=out)
    else:
        return _call_cpp_func(_cpp.sum, x, dim=dim, out=out)


def nansum(x, dim=None, out=None):
    """Element-wise sum over the specified dimension. NaNs are treated as zero.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the sum. If not
                given, the sum over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The sum of the input values with NaNs set to zero.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nansum, x, out=out)
    else:
        return _call_cpp_func(_cpp.nansum, x, dim=dim, out=out)


def min(x, dim=None, out=None):
    """Element-wise min over the specified dimension or all dimensions if not
    provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the min. If not
                given, the min over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The min of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.min, x, out=out)
    else:
        return _call_cpp_func(_cpp.min, x, dim=dim, out=out)


def max(x, dim=None, out=None):
    """Element-wise max over the specified dimension or all dimensions if not
    provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the max. If not
                given, the max over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The max of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.max, x, out=out)
    else:
        return _call_cpp_func(_cpp.max, x, dim=dim, out=out)


def nanmin(x, dim=None, out=None):
    """Element-wise min ignoring not at number values over the specified
    dimension or all dimensions if not provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the min. If not
                given, the min over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The min of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nanmin, x, out=out)
    else:
        return _call_cpp_func(_cpp.nanmin, x, dim=dim, out=out)


def nanmax(x, dim=None, out=None):
    """Element-wise max ignoring not a number values over the specified
    dimension or all dimensions if not provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the max. If not
                given, the max over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The max of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nanmax, x, out=out)
    else:
        return _call_cpp_func(_cpp.nanmax, x, dim=dim, out=out)


def all(x, dim=None, out=None):
    """Element-wise AND over the specified dimension or all dimensions if not
    provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the AND. If not
                given, the AND over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The AND of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.all, x, out=out)
    else:
        return _call_cpp_func(_cpp.all, x, dim=dim, out=out)


def any(x, dim=None, out=None):
    """Element-wise OR over the specified dimension or all dimensions if not
    provided.

    :param x: Input data.
    :param dim: Optional dimension along which to calculate the OR. If not
                given, the OR over all dimensions is calculated.
    :param out: Optional output buffer.
    :raises: If the dimension does not exist, or the dtype cannot be summed,
             e.g., if it is a string.
    :return: The OR of the input values.
    """
    if dim is None:
        return _call_cpp_func(_cpp.any, x, out=out)
    else:
        return _call_cpp_func(_cpp.any, x, dim=dim, out=out)
