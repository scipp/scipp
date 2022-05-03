# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from __future__ import annotations
from typing import Optional

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLikeType


def mean(x: VariableLikeType,
         dim: Optional[str] = None,
         *,
         out: Optional[VariableLikeType] = None) -> VariableLikeType:
    """Arithmetic mean of elements in the input.

    If the input has variances, the variances stored in the output are based on
    the "standard deviation of the mean", i.e.,
    :math:`\\sigma_{mean} = \\sigma / \\sqrt{N}`.
    :math:`N` is the length of the input dimension.
    :math:`\\sigma` is estimated as the average of the standard deviations of
    the input elements along that dimension.

    See :py:func:`scipp.sum` on how rounding errors for float32 inputs are handled.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension along which to calculate the mean. If not
        given, the mean over all dimensions is calculated.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as x
        The mean of the input values.

    See Also
    --------
    scipp.nanmean:
        Ignore NaN's when calculating the mean.
    """
    if dim is None:
        return _call_cpp_func(_cpp.mean, x, out=out)
    else:
        return _call_cpp_func(_cpp.mean, x, dim=dim, out=out)


def nanmean(x: VariableLikeType,
            dim: Optional[str] = None,
            *,
            out: Optional[VariableLikeType] = None) -> VariableLikeType:
    """Arithmetic mean of elements in the input ignoring NaN's.

    If the input has variances, the variances stored in the output are based on
    the "standard deviation of the mean", i.e.,
    :math:`\\sigma_{mean} = \\sigma / \\sqrt{N}`.
    :math:`N` is the length of the input dimension.
    :math:`\\sigma` is estimated as the average of the standard deviations of
    the input elements along that dimension.

    See :py:func:`scipp.sum` on how rounding errors for float32 inputs are handled.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension along which to calculate the mean. If not
        given, the nanmean over all dimensions is calculated.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as x
        The mean of the input values which are not NaN.

    See Also
    --------
    scipp.mean:
        Compute the mean without special handling of NaN.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nanmean, x, out=out)
    else:
        return _call_cpp_func(_cpp.nanmean, x, dim=dim, out=out)


def sum(x: VariableLikeType,
        dim: Optional[str] = None,
        *,
        out: Optional[VariableLikeType] = None) -> VariableLikeType:
    """Sum of elements in the input.

    If the input data is in single precision (dtype='float32') this internally uses
    double precision (dtype='float64') to reduce the effect of accumulated rounding
    errors. If multiple dimensions are reduced, the current implementation casts back
    to float32 after handling each dimension, i.e., the result is equivalent to what
    would be obtained from manually summing individual dimensions.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Optional dimension along which to calculate the sum. If not
        given, the sum over all dimensions is calculated.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as x
        The sum of the input values.

    See Also
    --------
    scipp.nansum:
        Ignore NaN's when calculating the sum.
    """
    if dim is None:
        return _call_cpp_func(_cpp.sum, x, out=out)
    else:
        return _call_cpp_func(_cpp.sum, x, dim=dim, out=out)


def nansum(x: VariableLikeType,
           dim: Optional[str] = None,
           *,
           out: Optional[VariableLikeType] = None) -> VariableLikeType:
    """Sum of elements in the input ignoring NaN's.

    See :py:func:`scipp.sum` on how rounding errors for float32 inputs are handled.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Optional dimension along which to calculate the sum. If not
        given, the sum over all dimensions is calculated.
    out:
        Optional output buffer.

    Returns
    -------
    : Same type as x
        The sum of the input values which are not NaN.

    See Also
    --------
    scipp.sum:
       Compute the sum without special handling of NaN.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nansum, x, out=out)
    else:
        return _call_cpp_func(_cpp.nansum, x, dim=dim, out=out)


def min(x: _cpp.Variable,
        dim: Optional[str] = None,
        *,
        out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Minimum of elements in the input.

    Parameters
    ----------
    x:
        Input data.
    dim:
        Optional dimension along which to calculate the min. If not
        given, the min over all dimensions is calculated.
    out:
        Optional output buffer.

    Returns
    -------
    :
        The minimum of the input values.

    See Also
    --------
    scipp.max:
        Element-wise maximum.
    scipp.nanmin:
        Same as min but ignoring NaN's.
    scipp.nanmax:
        Same as max but ignoring NaN's.
    """
    if dim is None:
        return _call_cpp_func(_cpp.min, x, out=out)
    else:
        return _call_cpp_func(_cpp.min, x, dim=dim, out=out)


def max(x: _cpp.Variable,
        dim: Optional[str] = None,
        *,
        out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Maximum of elements in the input.

    Parameters
    ----------
    x:
        Input data.
    dim:
        Optional dimension along which to calculate the max. If not
        given, the max over all dimensions is calculated.
    out:
        Optional output buffer.

    Returns
    -------
    :
        The maximum of the input values.

    See Also
    --------
    scipp.min:
        Element-wise minimum.
    scipp.nanmin:
        Same as min but ignoring NaN's.
    scipp.nanmax:
        Same as max but ignoring NaN's.
    """
    if dim is None:
        return _call_cpp_func(_cpp.max, x, out=out)
    else:
        return _call_cpp_func(_cpp.max, x, dim=dim, out=out)


def nanmin(x: _cpp.Variable,
           dim: Optional[str] = None,
           *,
           out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Minimum of elements in the input ignoring NaN's.

    Parameters
    ----------
    x:
        Input data.
    dim:
        Optional dimension along which to calculate the min. If not
        given, the min over all dimensions is calculated.
    out:
        Optional output buffer.

    Returns
    -------
    :
        The minimum of the input values.

    See Also
    --------
    scipp.min:
        Element-wise minimum without special handling for NaN.
    scipp.max:
        Element-wise maximum without special handling for NaN.
    scipp.nanmax:
        Same as max but ignoring NaN's.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nanmin, x, out=out)
    else:
        return _call_cpp_func(_cpp.nanmin, x, dim=dim, out=out)


def nanmax(x: _cpp.Variable,
           dim: Optional[str] = None,
           *,
           out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Maximum of elements in the input ignoring NaN's.

    Parameters
    ----------
    x:
        Input data.
    dim:
        Optional dimension along which to calculate the max. If not
        given, the max over all dimensions is calculated.
    out:
        Optional output buffer.

    Returns
    -------
    :
        The maximum of the input values.

    See Also
    --------
    scipp.max:
        Element-wise maximum without special handling for NaN.
    scipp.min:
        Element-wise minimum without special handling for NaN.
    scipp.nanmin:
        Same as min but ignoring NaN's.
    """
    if dim is None:
        return _call_cpp_func(_cpp.nanmax, x, out=out)
    else:
        return _call_cpp_func(_cpp.nanmax, x, dim=dim, out=out)


def all(x: _cpp.Variable,
        dim: Optional[str] = None,
        *,
        out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Logical AND over input values.

    Parameters
    ----------
    x:
        Input data.
    dim:
        Optional dimension along which to calculate the AND. If not
        given, the AND over all dimensions is calculated.
    out:
        Optional output buffer.

    Returns
    -------
    :
        A variable containing ``True`` if all input values (along the given dimension)
        are ``True``.

    See Also
    --------
    scipp.any:
        Logical OR.
    """
    if dim is None:
        return _call_cpp_func(_cpp.all, x, out=out)
    else:
        return _call_cpp_func(_cpp.all, x, dim=dim, out=out)


def any(x: _cpp.Variable,
        dim: Optional[str] = None,
        *,
        out: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Logical OR over input values.

    Parameters
    ----------
    x:
        Input data.
    dim:
        Optional dimension along which to calculate the OR. If not
        given, the OR over all dimensions is calculated.
    out:
        Optional output buffer.

    Returns
    -------
    :
        A variable containing ``True`` if any input values (along the given dimension)
        are ``True``.

    See Also
    --------
    scipp.all:
        Logical AND.
    """
    if dim is None:
        return _call_cpp_func(_cpp.any, x, out=out)
    else:
        return _call_cpp_func(_cpp.any, x, dim=dim, out=out)
