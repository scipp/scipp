# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from __future__ import annotations
from typing import Optional

from .._scipp import core as _cpp
from ..typing import VariableLikeType


def mean(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
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
        return _cpp.mean(x)
    else:
        return _cpp.mean(x, dim=dim)


def nanmean(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
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
        return _cpp.nanmean(x)
    else:
        return _cpp.nanmean(x, dim=dim)


def sum(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
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
        return _cpp.sum(x)
    else:
        return _cpp.sum(x, dim=dim)


def nansum(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
    """Sum of elements in the input ignoring NaN's.

    See :py:func:`scipp.sum` on how rounding errors for float32 inputs are handled.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Optional dimension along which to calculate the sum. If not
        given, the sum over all dimensions is calculated.

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
        return _cpp.nansum(x)
    else:
        return _cpp.nansum(x, dim=dim)


def min(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
    """Minimum of elements in the input.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Optional dimension along which to calculate the min. If not
        given, the min over all dimensions is calculated.

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
        return _cpp.min(x)
    else:
        return _cpp.min(x, dim=dim)


def max(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
    """Maximum of elements in the input.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Optional dimension along which to calculate the max. If not
        given, the max over all dimensions is calculated.

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
        return _cpp.max(x)
    else:
        return _cpp.max(x, dim=dim)


def nanmin(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
    """Minimum of elements in the input ignoring NaN's.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Optional dimension along which to calculate the min. If not
        given, the min over all dimensions is calculated.

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
        return _cpp.nanmin(x)
    else:
        return _cpp.nanmin(x, dim=dim)


def nanmax(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
    """Maximum of elements in the input ignoring NaN's.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Optional dimension along which to calculate the max. If not
        given, the max over all dimensions is calculated.

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
        return _cpp.nanmax(x)
    else:
        return _cpp.nanmax(x, dim=dim)


def all(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
    """Logical AND over input values.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Optional dimension along which to calculate the AND. If not
        given, the AND over all dimensions is calculated.

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
        return _cpp.all(x)
    else:
        return _cpp.all(x, dim=dim)


def any(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
    """Logical OR over input values.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Optional dimension along which to calculate the OR. If not
        given, the OR over all dimensions is calculated.

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
        return _cpp.any(x)
    else:
        return _cpp.any(x, dim=dim)
