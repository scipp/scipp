# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from __future__ import annotations

import builtins
from typing import Optional, Union

import numpy as np

from .._scipp import core as _cpp
from ..typing import Dims, VariableLikeType
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .cpp_classes import Masks
from .data_group import DataGroup, data_group_nary
from .variable import array


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
        return _call_cpp_func(_cpp.mean, x)
    else:
        return _call_cpp_func(_cpp.mean, x, dim=dim)


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
        return _call_cpp_func(_cpp.nanmean, x)
    else:
        return _call_cpp_func(_cpp.nanmean, x, dim=dim)


def median(x: VariableLikeType, dim: Dims = None) -> VariableLikeType:
    # TODO warn about costs (numpy, numpy.ma -> broadcast)
    # TODO explain all masked
    if isinstance(x, _cpp.Dataset):
        return _cpp.Dataset({k: median(v, dim=dim) for k, v in x.items()})
    if isinstance(x, DataGroup):
        return data_group_nary(median, x, dim=dim)

    _expect_no_variance(x, 'median')
    reduced_dims = _normalize_reduced_dims(x, dim)
    axis = _dims_to_axis(x, reduced_dims)
    out_dims = [d for d in x.dims if d not in reduced_dims]
    if isinstance(x, _cpp.Variable):
        return array(dims=out_dims, values=np.median(x.values, axis=axis), unit=x.unit)
    if isinstance(x, _cpp.DataArray):
        mask, preserved_masks = _merge_masks(x.masks, reduced_dims=reduced_dims)
        if mask is not None:
            masked = np.ma.masked_array(
                x.values, mask=mask.broadcast(x.dims, x.shape).values
            )
            res = np.ma.median(masked, axis=axis)
        else:
            res = np.median(x.values, axis=axis)
        return _cpp.DataArray(
            array(dims=out_dims, values=res, unit=x.unit),
            coords={
                key: coord
                for key, coord in x.coords.items()
                if builtins.all(d in out_dims for d in coord.dims)
            },
            masks=preserved_masks,
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
        return _call_cpp_func(_cpp.sum, x)
    elif isinstance(dim, str):
        return _call_cpp_func(_cpp.sum, x, dim=dim)
    for d in dim:
        x = _call_cpp_func(_cpp.sum, x, d)
    return x


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
        return _call_cpp_func(_cpp.nansum, x)
    else:
        return _call_cpp_func(_cpp.nansum, x, dim=dim)


def min(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
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
        return _call_cpp_func(_cpp.min, x)
    else:
        return _call_cpp_func(_cpp.min, x, dim=dim)


def max(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
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
        return _call_cpp_func(_cpp.max, x)
    else:
        return _call_cpp_func(_cpp.max, x, dim=dim)


def nanmin(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
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
        return _call_cpp_func(_cpp.nanmin, x)
    else:
        return _call_cpp_func(_cpp.nanmin, x, dim=dim)


def nanmax(x: VariableLikeType, dim: Optional[str] = None) -> VariableLikeType:
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
        return _call_cpp_func(_cpp.nanmax, x)
    else:
        return _call_cpp_func(_cpp.nanmax, x, dim=dim)


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
        return _call_cpp_func(_cpp.all, x)
    else:
        return _call_cpp_func(_cpp.all, x, dim=dim)


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
        return _call_cpp_func(_cpp.any, x)
    else:
        return _call_cpp_func(_cpp.any, x, dim=dim)


def _normalize_reduced_dims(x: VariableLikeType, dim: Dims) -> tuple[str, ...]:
    if dim is None:
        return x.dims
    if isinstance(dim, str):
        return (dim,)
    return tuple(dim)


def _dims_to_axis(
    x: VariableLikeType, dim: tuple[str, ...]
) -> Union[int, tuple[int, ...]]:
    return tuple(_dim_index(x.dims, d) for d in dim)


def _dim_index(dims: tuple[str, ...], dim: str) -> int:
    try:
        return dims.index(dim)
    except ValueError:
        raise _cpp.DimensionError(
            f'Expected dimension to be in {dims}, got {dim}'
        ) from None


def _expect_no_variance(x: VariableLikeType, op: str) -> None:
    if x.variances is not None:
        raise _cpp.VariancesError(f"'{op}' does not support variances")


def _merge_masks(
    masks: Masks, reduced_dims: tuple[str, ...]
) -> tuple[Optional[_cpp.Variable], dict[str, _cpp.Variable]]:
    merged = None
    preserved = {}
    for key, mask in masks.items():
        if builtins.any(dim in reduced_dims for dim in mask.dims):
            if merged is None:
                merged = mask
            else:
                merged = merged | mask
        else:
            preserved[key] = mask
    return merged, preserved
