# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from __future__ import annotations

from collections.abc import Callable
from typing import Any, NoReturn, cast

import numpy as np
import numpy.typing as npt

from .._scipp import core as _cpp
from ..typing import Dims, VariableLike, VariableLikeType
from . import concepts
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .cpp_classes import (
    DataArray,
    Dataset,
    DimensionError,
    DTypeError,
    Unit,
    Variable,
    VariancesError,
)
from .data_group import DataGroup, data_group_nary
from .variable import array


def _apply_op(
    x: VariableLike, dim: Dims, func: Callable[..., VariableLike]
) -> VariableLike:
    if dim is None:
        return _call_cpp_func(func, x)
    elif isinstance(dim, str):
        return _call_cpp_func(func, x, dim=dim)
    for d in dim:
        x = _call_cpp_func(func, x, dim=d)
    return x


def mean(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
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
        Dimension(s) along which to calculate the mean.
        If not given, the mean over all dimensions is calculated.

    Returns
    -------
    : Same type as x
        The mean of the input values.

    See Also
    --------
    scipp.var:
        Compute the variance.
    scipp.std:
        Compute the standard deviation.
    scipp.nanmean:
        Ignore NaN's when calculating the mean.

    Examples
    --------
    >>> import scipp as sc
    >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0], unit='m')
    >>> sc.mean(x)
    <scipp.Variable> ()    float64              [m]  2.5

    Calculate mean along a specific dimension:

    >>> x = sc.array(dims=['x', 'y'], values=[[1, 2, 3], [4, 5, 6]])
    >>> sc.mean(x, 'y')
    <scipp.Variable> (x: 2)    float64  [dimensionless]  [2, 5]

    >>> sc.mean(x, 'x')
    <scipp.Variable> (y: 3)    float64  [dimensionless]  [2.5, 3.5, 4.5]

    With variances, the output variance is based on the standard error of the mean:

    >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0],
    ...              variances=[0.1, 0.1, 0.1, 0.1], unit='m')
    >>> result = sc.mean(x)
    >>> result
    <scipp.Variable> ()    float64              [m]  2.5  0.025
    >>> result.variances is not None  # variance decreases by factor of N
    True
    """
    return _apply_op(x, dim, _cpp.mean)  # type: ignore[return-value]


def nanmean(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
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
        Dimension(s) along which to calculate the mean.
        If not given, the mean over all dimensions is calculated.

    Returns
    -------
    : Same type as x
        The mean of the input values which are not NaN.

    See Also
    --------
    scipp.nanvar:
        Compute the variance, ignoring NaN's.
    scipp.nanstd:
        Compute the standard deviation, ignoring NaN's.
    scipp.mean:
        Compute the mean without special handling of NaN.

    Examples
    --------
    >>> import scipp as sc
    >>> import numpy as np
    >>> x = sc.array(dims=['x'], values=[1.0, np.nan, 3.0, 4.0])
    >>> sc.nanmean(x)
    <scipp.Variable> ()    float64  [dimensionless]  2.66667

    If all values are NaN, the result is NaN:

    >>> x = sc.array(dims=['x'], values=[np.nan, np.nan, np.nan])
    >>> sc.nanmean(x)
    <scipp.Variable> ()    float64  [dimensionless]  ...nan
    """
    return _apply_op(x, dim, _cpp.nanmean)  # type: ignore[return-value]


def median(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    """Compute the median of the input values.

    The median is the middle value of a sorted copy of the input array
    along each reduced dimension.
    That is, for an array of ``N`` unmasked values, the median is

    - odd ``N``: ``x[(N-1)/2]``
    - even ``N``: ``(x[N/2-1] + x[N/2]) / 2``

    Note
    ----
    Masks are broadcast to the shape of ``x``.
    This can lead to a large temporary memory usage.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the median.
        If not given, the median over a flattened version of the array is calculated.

    Returns
    -------
    : Same type as x
        The median of the input values.

    Raises
    ------
    scipp.VariancesError
        If the input has variances.
    scipp.DTypeError
        If the input is binned or does otherwise not support computing medians.

    See Also
    --------
    scipp.nanmedian:
        Ignore NaN's when calculating the median.

    Examples
    --------
    ``median`` is available as a method:

        >>> x = sc.array(dims=['x'], values=[2, 5, 1, 8, 4])
        >>> x.median()
        <scipp.Variable> ()    float64  [dimensionless]  4
        >>> x = sc.array(dims=['x'], values=[2, 5, 1, 8])
        >>> x.median()
        <scipp.Variable> ()    float64  [dimensionless]  3.5

    The median can be computed along a given dimension:

        >>> x = sc.array(dims=['x', 'y'], values=[[1, 3, 6], [2, 7, 4]])
        >>> x.median('y')
        <scipp.Variable> (x: 2)    float64  [dimensionless]  [3, 4]

    Masked elements are ignored:

        >>> x = sc.DataArray(
        ...     sc.array(dims=['x'], values=[5, 3, 4, 3]),
        ...     masks={'m': sc.array(dims=['x'], values=[False, True, False, False])}
        ... )
        >>> x.median()
        <scipp.DataArray>
        Dimensions: Sizes[]
        Data:
                                    float64  [dimensionless]  ()  4

    Data with variances is not supported:

        >>> import scipp as sc
        >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], variances=[0.1, 0.1, 0.1])
        >>> sc.median(x)  # doctest: +IGNORE_EXCEPTION_DETAIL
        Traceback (most recent call last):
            ...
        scipp.core.VariancesError: 'median' does not support variances
    """
    r = _reduce_with_numpy(
        x,
        dim=dim,
        sc_func=cast(Callable[..., VariableLike], median),
        np_func=np.median,
        np_ma_func=np.ma.median,
        unit_func=lambda u: u,
        kwargs={},
    )
    return r


def nanmedian(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    """Compute the median of the input values ignoring NaN's.

    The median is the middle value of a sorted copy of the input array
    along each reduced dimension.
    That is, for an array of ``N`` unmasked, non-NaN values, the median is

    - odd ``N``: ``x[(N-1)/2]``
    - even ``N``: ``(x[N/2-1] + x[N/2]) / 2``

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the median.
        If not given, the median over a flattened version of the array is calculated.

    Returns
    -------
    : Same type as x
        The median of the input values.

    Raises
    ------
    scipp.VariancesError
        If the input has variances.
    scipp.DTypeError
        If the input is binned or does otherwise not support computing medians.
    ValueError
        If the input has masks.
        Mask out NaN's and then use :func:`scipp.median` instead.

    See Also
    --------
    scipp.median:
        Compute the median without special handling of NaN's.

    Examples
    --------
    ``nanmedian`` is available as a method:

        >>> import scipp as sc
        >>> import numpy as np
        >>> x = sc.array(dims=['x'], values=[2, 5, 1, np.nan, 8, 4])
        >>> x.nanmedian()
        <scipp.Variable> ()    float64  [dimensionless]  4
        >>> x = sc.array(dims=['x'], values=[2, np.nan, 5, 1, 8])
        >>> x.nanmedian()
        <scipp.Variable> ()    float64  [dimensionless]  3.5
    """

    def _catch_masked(*args: object, **kwargs: object) -> NoReturn:
        # Because there is no np.ma.nanmedian
        raise ValueError(
            'nanmedian does not support masked data arrays. '
            'Consider masking NaN values and calling scipp.median'
        )

    return _reduce_with_numpy(
        x,
        dim=dim,
        sc_func=cast(Callable[..., VariableLike], nanmedian),
        np_func=np.nanmedian,
        np_ma_func=_catch_masked,
        unit_func=lambda u: u,
        kwargs={},
    )


def var(x: VariableLikeType, dim: Dims = None, *, ddof: int) -> VariableLikeType:
    r"""Compute the variance of the input values.

    This function computes the variance of the input values which is *not*
    the same as the ``x.variances`` property but instead defined as

    .. math::

        \mathsf{var}(x) = \frac1{N - \mathsf{ddof}}
                          \sum_{i=1}^{N}\, {(x_i - \bar{x})}^2

    where :math:`x_i` are the unmasked ``values`` of the input and
    :math:`\bar{x}` is the mean, see :func:`scipp.mean`.
    See the ``ddof`` parameter description for what value to choose.

    Note
    ----
    Masks are broadcast to the shape of ``x``.
    This can lead to a large temporary memory usage.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the variance.
        If not given, the variance over a flattened version of the array is calculated.
    ddof:
        'Delta degrees of freedom'.
        For sample variances, set ``ddof=1`` to obtain an unbiased estimator.
        For normally distributed variables, set ``ddof=0`` to obtain a maximum
        likelihood estimate.
        See :func:`numpy.var` for more details.

        In contrast to NumPy, this is a required parameter in Scipp to
        avoid potentially hard-to-find mistakes based on implicit assumptions
        about what the input data represents.

    Returns
    -------
    : Same type as x
        The variance of the input values.

    Raises
    ------
    scipp.VariancesError
        If the input has variances.
    scipp.DTypeError
        If the input is binned or does otherwise not support computing variances.

    See Also
    --------
    scipp.variances:
        Extract the stored variances of a :class:`scipp.Variable`.
    scipp.mean:
        Compute the arithmetic mean.
    scipp.std:
        Compute the standard deviation.
    scipp.nanvar:
        Ignore NaN's when calculating the variance.

    Examples
    --------
    ``var`` is available as a method:

        >>> x = sc.array(dims=['x'], values=[3, 5, 2, 3])
        >>> x.var(ddof=0)
        <scipp.Variable> ()    float64  [dimensionless]  1.1875
        >>> x.var(ddof=1)
        <scipp.Variable> ()    float64  [dimensionless]  1.58333

    Select a dimension to reduce:

        >>> x = sc.array(dims=['x', 'y'], values=[[1, 3, 6], [2, 7, 4]])
        >>> x.var('y', ddof=0)
        <scipp.Variable> (x: 2)    float64  [dimensionless]  [4.22222, 4.22222]
        >>> x.var('x', ddof=0)
        <scipp.Variable> (y: 3)    float64  [dimensionless]  [0.25, 4, 1]
    """
    return _reduce_with_numpy(
        x,
        dim=dim,
        sc_func=cast(Callable[..., VariableLike], var),
        np_func=np.var,
        np_ma_func=np.ma.var,
        unit_func=lambda u: u**2,
        kwargs={'ddof': ddof},
    )


def nanvar(x: VariableLikeType, dim: Dims = None, *, ddof: int) -> VariableLikeType:
    r"""Compute the variance of the input values ignoring NaN's.

    This function computes the variance of the input values which is *not*
    the same as the ``x.variances`` property but instead defined as

    .. math::

        \mathsf{nanvar}(x) = \frac1{N - \mathsf{ddof}}
                             \sum_{i=1}^{N}\, {(x_i - \bar{x})}^2

    where :math:`x_i` are the non-NaN ``values`` of the input and
    :math:`\bar{x}` is the mean, see :func:`scipp.nanmean`.
    See the ``ddof`` parameter description for what value to choose.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the variance.
        If not given, the variance over a flattened version of the array is calculated.
    ddof:
        'Delta degrees of freedom'.
        For sample variances, set ``ddof=1`` to obtain an unbiased estimator.
        For normally distributed variables, set ``ddof=0`` to obtain a maximum
        likelihood estimate.
        See :func:`numpy.nanvar` for more details.

        In contrast to NumPy, this is a required parameter in Scipp to
        avoid potentially hard-to-find mistakes based on implicit assumptions
        about what the input data represents.

    Returns
    -------
    : Same type as x
        The variance of the non-NaN input values.

    Raises
    ------
    scipp.VariancesError
        If the input has variances.
    scipp.DTypeError
        If the input is binned or does otherwise not support computing variances.
    ValueError
        If the input has masks.
        Mask out NaN's and then use :func:`scipp.var` instead.

    See Also
    --------
    scipp.nanmean:
        Compute the arithmetic mean ignoring NaN's.
    scipp.nanstd:
        Compute the standard deviation, ignoring NaN's.
    scipp.var:
        Compute the variance without special handling of NaN's.

    Examples
    --------
    ``nanvar`` is available as a method:

        >>> import scipp as sc
        >>> import numpy as np
        >>> x = sc.array(dims=['x'], values=[np.nan, 5, 2, 3])
        >>> x.nanvar(ddof=0)
        <scipp.Variable> ()    float64  [dimensionless]  1.55556
        >>> x.nanvar(ddof=1)
        <scipp.Variable> ()    float64  [dimensionless]  2.33333
    """

    def _catch_masked(*args: object, **kwargs: object) -> NoReturn:
        # Because there is no np.ma.nanvar
        raise ValueError(
            'nanvar does not support masked data arrays. '
            'Consider masking NaN values and calling scipp.var'
        )

    return _reduce_with_numpy(
        x,
        dim=dim,
        sc_func=cast(Callable[..., VariableLike], nanvar),
        np_func=np.nanvar,
        np_ma_func=_catch_masked,
        unit_func=lambda u: u**2,
        kwargs={'ddof': ddof},
    )


def std(x: VariableLikeType, dim: Dims = None, *, ddof: int) -> VariableLikeType:
    r"""Compute the standard deviation of the input values.

    This function computes the standard deviation of the input values which is *not*
    related to the ``x.variances`` property but instead defined as

    .. math::

        \mathsf{std}(x)^2 = \frac1{N - \mathsf{ddof}}
                            \sum_{i=1}^{N}\, {(x_i - \bar{x})}^2

    where :math:`x_i` are the unmasked ``values`` of the input and
    :math:`\bar{x}` is the mean, see :func:`scipp.mean`.
    See the ``ddof`` parameter description for what value to choose.

    Note
    ----
    Masks are broadcast to the shape of ``x``.
    This can lead to a large temporary memory usage.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the standard deviation.
        If not given, the standard deviation over a flattened version
        of the array is calculated.
    ddof:
        'Delta degrees of freedom'.
        For sample standard deviations, set ``ddof=1`` to obtain an unbiased estimator.
        For normally distributed variables, set ``ddof=0`` to obtain a maximum
        likelihood estimate.
        See :func:`numpy.std` for more details.

        In contrast to NumPy, this is a required parameter in Scipp to
        avoid potentially hard-to-find mistakes based on implicit assumptions
        about what the input data represents.

    Returns
    -------
    : Same type as x
        The standard deviation of the input values.

    Raises
    ------
    scipp.VariancesError
        If the input has variances.
    scipp.DTypeError
        If the input is binned or does
        otherwise not support computing standard deviations.

    See Also
    --------
    scipp.stddevs:
        Compute the standard deviations from the stored
        variances of a :class:`scipp.Variable`.
    scipp.mean:
        Compute the arithmetic mean.
    scipp.var:
        Compute the variance.
    scipp.nanstd:
        Ignore NaN's when calculating the standard deviation.

    Examples
    --------
    ``std`` is available as a method:

        >>> x = sc.array(dims=['x'], values=[3, 5, 2, 3])
        >>> x.std(ddof=0)
        <scipp.Variable> ()    float64  [dimensionless]  1.08972
        >>> x.std(ddof=1)
        <scipp.Variable> ()    float64  [dimensionless]  1.25831

    Select a dimension to reduce:

        >>> x = sc.array(dims=['x', 'y'], values=[[1, 3, 6], [2, 7, 4]])
        >>> x.std('y', ddof=0)
        <scipp.Variable> (x: 2)    float64  [dimensionless]  [2.0548, 2.0548]
        >>> x.std('x', ddof=0)
        <scipp.Variable> (y: 3)    float64  [dimensionless]  [0.5, 2, 1]
    """
    return _reduce_with_numpy(
        x,
        dim=dim,
        sc_func=cast(Callable[..., VariableLike], std),
        np_func=np.std,
        np_ma_func=np.ma.std,
        unit_func=lambda u: u,
        kwargs={'ddof': ddof},
    )


def nanstd(x: VariableLikeType, dim: Dims = None, *, ddof: int) -> VariableLikeType:
    r"""Compute the standard deviation of the input values ignoring NaN's.

    This function computes the standard deviation of the input values which is *not*
    related to the ``x.variances`` property but instead defined as

    .. math::

        \mathsf{nanstd}(x)^2 = \frac1{N - \mathsf{ddof}}
                               \sum_{i=1}^{N}\, {(x_i - \bar{x})}^2

    where :math:`x_i` are the non-NaN ``values`` of the input and
    :math:`\bar{x}` is the mean, see :func:`scipp.nanmean`.
    See the ``ddof`` parameter description for what value to choose.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the standard deviation.
        If not given, the standard deviation over a flattened version
        of the array is calculated.
    ddof:
        'Delta degrees of freedom'.
        For sample standard deviations, set ``ddof=1`` to obtain an unbiased estimator.
        For normally distributed variables, set ``ddof=0`` to obtain a maximum
        likelihood estimate.
        See :func:`numpy.nanstd` for more details.

        In contrast to NumPy, this is a required parameter in Scipp to
        avoid potentially hard-to-find mistakes based on implicit assumptions
        about what the input data represents.

    Returns
    -------
    : Same type as x
        The standard deviation of the input values.

    Raises
    ------
    scipp.VariancesError
        If the input has variances.
    scipp.DTypeError
        If the input is binned or does
        otherwise not support computing standard deviations.
    ValueError
        If the input has masks.
        Mask out NaN's and then use :func:`scipp.std` instead.

    See Also
    --------
    scipp.nanmean:
        Compute the arithmetic mean ignoring NaN's.
    scipp.nanvar:
        Compute the variance, ignoring NaN's.
    scipp.std:
        Compute the standard deviation without special handling of NaN's.

    Examples
    --------
    ``nanstd`` is available as a method:

        >>> import scipp as sc
        >>> import numpy as np
        >>> x = sc.array(dims=['x'], values=[np.nan, 5, 2, 3])
        >>> x.nanstd(ddof=0)
        <scipp.Variable> ()    float64  [dimensionless]  1.24722
        >>> x.nanstd(ddof=1)
        <scipp.Variable> ()    float64  [dimensionless]  1.52753
    """

    def _catch_masked(*args: object, **kwargs: object) -> NoReturn:
        # Because there is no np.ma.nanstd
        raise ValueError(
            'nanstd does not support masked data arrays. '
            'Consider masking NaN values and calling scipp.std'
        )

    return _reduce_with_numpy(
        x,
        dim=dim,
        sc_func=cast(Callable[..., VariableLike], nanstd),
        np_func=np.nanstd,
        np_ma_func=_catch_masked,
        unit_func=lambda u: u,
        kwargs={'ddof': ddof},
    )


def sum(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
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
        Dimension(s) along which to calculate the sum.
        If not given, the sum over all dimensions is calculated.

    Returns
    -------
    : Same type as x
        The sum of the input values.

    See Also
    --------
    scipp.nansum:
        Ignore NaN's when calculating the sum.

    Examples
    --------
    >>> import scipp as sc
    >>> x = sc.array(dims=['x'], values=[1, 2, 3, 4], unit='m')
    >>> sc.sum(x)
    <scipp.Variable> ()      int64              [m]  10

    Sum along a specific dimension:

    >>> x = sc.array(dims=['x', 'y'], values=[[1, 2, 3], [4, 5, 6]])
    >>> sc.sum(x, 'y')
    <scipp.Variable> (x: 2)      int64  [dimensionless]  [6, 15]
    >>> sc.sum(x, 'x')
    <scipp.Variable> (y: 3)      int64  [dimensionless]  [5, 7, 9]
    """
    return _apply_op(x, dim, _cpp.sum)  # type: ignore[return-value]


def nansum(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    """Sum of elements in the input ignoring NaN's.

    See :py:func:`scipp.sum` on how rounding errors for float32 inputs are handled.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the sum.
        If not given, the sum over all dimensions is calculated.

    Returns
    -------
    : Same type as x
        The sum of the input values which are not NaN.

    See Also
    --------
    scipp.sum:
       Compute the sum without special handling of NaN.

    Examples
    --------
    >>> import scipp as sc
    >>> import numpy as np
    >>> x = sc.array(dims=['x'], values=[1.0, np.nan, 3.0, 4.0])
    >>> sc.nansum(x)
    <scipp.Variable> ()    float64  [dimensionless]  8
    """
    return _apply_op(x, dim, _cpp.nansum)  # type: ignore[return-value]


def min(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    """Minimum of elements in the input.

    Warning
    -------

    Scipp returns DBL_MAX or INT_MAX for empty inputs of float or int dtype,
    respectively, while NumPy raises. Note that in the case of :py:class:`DataArray`,
    inputs can also be "empty" if all elements contributing to an output element are
    masked.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the min.
        If not given, the min over all dimensions is calculated.

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

    Examples
    --------
    >>> import scipp as sc
    >>> x = sc.array(dims=['x'], values=[3, 1, 4, 1, 5])
    >>> sc.min(x)
    <scipp.Variable> ()      int64  [dimensionless]  1

    Minimum along a specific dimension:

    >>> x = sc.array(dims=['x', 'y'], values=[[3, 1, 4], [1, 5, 9]])
    >>> sc.min(x, 'y')
    <scipp.Variable> (x: 2)      int64  [dimensionless]  [1, 1]
    """
    return _apply_op(x, dim, _cpp.min)  # type: ignore[return-value]


def max(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    """Maximum of elements in the input.

    Warning
    -------

    Scipp returns DBL_MIN or INT_MIN for empty inputs of float or int dtype,
    respectively, while NumPy raises. Note that in the case of :py:class:`DataArray`,
    inputs can also be "empty" if all elements contributing to an output element are
    masked.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the max.
        If not given, the max over all dimensions is calculated.

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

    Examples
    --------
    >>> import scipp as sc
    >>> x = sc.array(dims=['x'], values=[3, 1, 4, 1, 5])
    >>> sc.max(x)
    <scipp.Variable> ()      int64  [dimensionless]  5

    Maximum along a specific dimension:

    >>> x = sc.array(dims=['x', 'y'], values=[[3, 1, 4], [1, 5, 9]])
    >>> sc.max(x, 'y')
    <scipp.Variable> (x: 2)      int64  [dimensionless]  [4, 9]
    """
    return _apply_op(x, dim, _cpp.max)  # type: ignore[return-value]


def nanmin(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    """Minimum of elements in the input ignoring NaN's.

    Warning
    -------

    Scipp returns DBL_MAX or INT_MAX for empty inputs of float or int dtype,
    respectively, while NumPy raises. Note that in the case of :py:class:`DataArray`,
    inputs can also be "empty" if all elements contributing to an output element are
    masked. The same applies if all elements are NaN (or masked).

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the min.
        If not given, the min over all dimensions is calculated.

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

    Examples
    --------
    >>> import scipp as sc
    >>> import numpy as np
    >>> x = sc.array(dims=['x'], values=[3.0, np.nan, 1.0, 4.0])
    >>> sc.nanmin(x)
    <scipp.Variable> ()    float64  [dimensionless]  1
    """
    return _apply_op(x, dim, _cpp.nanmin)  # type: ignore[return-value]


def nanmax(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    """Maximum of elements in the input ignoring NaN's.

    Warning
    -------

    Scipp returns DBL_MIN or INT_MIN for empty inputs of float or int dtype,
    respectively, while NumPy raises. Note that in the case of :py:class:`DataArray`,
    inputs can also be "empty" if all elements contributing to an output element are
    masked. The same applies if all elements are NaN (or masked).

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the max.
        If not given, the max over all dimensions is calculated.

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

    Examples
    --------
    >>> import scipp as sc
    >>> import numpy as np
    >>> x = sc.array(dims=['x'], values=[3.0, np.nan, 1.0, 5.0])
    >>> sc.nanmax(x)
    <scipp.Variable> ()    float64  [dimensionless]  5
    """
    return _apply_op(x, dim, _cpp.nanmax)  # type: ignore[return-value]


def all(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    """Logical AND over input values.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the AND.
        If not given, the AND over all dimensions is calculated.

    Returns
    -------
    :
        A variable containing ``True`` if all input values (along the given dimension)
        are ``True``.

    See Also
    --------
    scipp.any:
        Logical OR.

    Examples
    --------
    >>> import scipp as sc
    >>> x = sc.array(dims=['x'], values=[True, True, True])
    >>> sc.all(x)
    <scipp.Variable> ()       bool        <no unit>  True
    >>> y = sc.array(dims=['x'], values=[True, False, True])
    >>> sc.all(y)
    <scipp.Variable> ()       bool        <no unit>  False

    Along a dimension:

    >>> x = sc.array(dims=['x', 'y'], values=[[True, False], [True, True]])
    >>> sc.all(x, 'y')
    <scipp.Variable> (x: 2)       bool        <no unit>  [False, True]
    """
    return _apply_op(x, dim, _cpp.all)  # type: ignore[return-value]


def any(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    """Logical OR over input values.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.
    dim:
        Dimension(s) along which to calculate the OR.
        If not given, the OR over all dimensions is calculated.

    Returns
    -------
    :
        A variable containing ``True`` if any input values (along the given dimension)
        are ``True``.

    See Also
    --------
    scipp.all:
        Logical AND.

    Examples
    --------
    >>> import scipp as sc
    >>> x = sc.array(dims=['x'], values=[False, False, False])
    >>> sc.any(x)
    <scipp.Variable> ()       bool        <no unit>  False
    >>> y = sc.array(dims=['x'], values=[False, True, False])
    >>> sc.any(y)
    <scipp.Variable> ()       bool        <no unit>  True

    Along a dimension:

    >>> x = sc.array(dims=['x', 'y'], values=[[False, True], [False, False]])
    >>> sc.any(x, 'y')
    <scipp.Variable> (x: 2)       bool        <no unit>  [True, False]
    """
    return _apply_op(x, dim, _cpp.any)  # type: ignore[return-value]


# Note: When passing `sc_func`, make sure to disassociate type vars of that function
# from the calling function. E.g., in `median`, use
#   sc_func=cast(Callable[..., VariableLike], median)
# This ensures that the return type of the `median` function is deduced correctly.
def _reduce_with_numpy(
    x: VariableLikeType,
    *,
    dim: Dims = None,
    sc_func: Callable[..., VariableLike],
    np_func: Callable[..., npt.NDArray[Any]],
    np_ma_func: Callable[..., npt.NDArray[Any]],
    unit_func: Callable[[Unit], Unit],
    kwargs: dict[str, Any],
) -> VariableLikeType:
    if isinstance(x, Dataset):
        return Dataset({k: sc_func(v, dim=dim, **kwargs) for k, v in x.items()})  # type: ignore[arg-type]
    if isinstance(x, DataGroup):
        return data_group_nary(sc_func, x, dim=dim, **kwargs)

    _expect_no_variance(x, sc_func.__name__)
    _expect_not_binned(x, sc_func.__name__)
    reduced_dims, out_dims, axis = _split_dims(x, dim)
    if isinstance(x, Variable):
        return array(
            dims=out_dims,
            values=np_func(x.values, axis=axis, **kwargs),
            unit=unit_func(x.unit) if x.unit is not None else None,
        )
    if isinstance(x, DataArray):
        if (mask := concepts.irreducible_mask(x, dim)) is not None:
            masked = np.ma.masked_where(  # type: ignore[no-untyped-call]
                mask.broadcast(dims=x.dims, shape=x.shape).values, x.values
            )
            res = np_ma_func(masked, axis=axis, **kwargs)
        else:
            res = np_func(x.values, axis=axis, **kwargs)
        return concepts.rewrap_reduced_data(
            x, array(dims=out_dims, values=res, unit=x.unit), dim
        )
    raise TypeError(f'invalid argument of type {type(x)} to {sc_func}')


def _dims_to_axis(x: VariableLikeType, dim: tuple[str, ...]) -> tuple[int, ...]:
    return tuple(_dim_index(x.dims, d) for d in dim)


def _dim_index(dims: tuple[str, ...], dim: str) -> int:
    try:
        return dims.index(dim)
    except ValueError:
        raise DimensionError(f'Expected dimension to be in {dims}, got {dim}') from None


def _split_dims(
    x: VariableLikeType, dim: Dims
) -> tuple[tuple[str, ...], tuple[str, ...], tuple[int, ...]]:
    reduced_dims = concepts.concrete_dims(x, dim)
    out_dims = tuple(d for d in x.dims if d not in reduced_dims)
    axis = _dims_to_axis(x, reduced_dims)
    return reduced_dims, out_dims, axis


def _expect_no_variance(x: VariableLikeType, op: str) -> None:
    if getattr(x, 'variances', None) is not None:
        raise VariancesError(f"'{op}' does not support variances")


def _expect_not_binned(x: VariableLikeType, op: str) -> None:
    if getattr(x, 'bins', None) is not None:
        raise DTypeError(f"'{op}' does not support binned data")
