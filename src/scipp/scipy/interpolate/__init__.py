# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for objects used in interpolation.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.interpolate`.
"""

from __future__ import annotations

from typing import Any, Literal, Protocol, TypeVar

import numpy as np
import numpy.typing as npt

from ...compat.wrapping import wrap1d
from ...core import (
    DataArray,
    DimensionError,
    DType,
    UnitError,
    Variable,
    empty,
    epoch,
    irreducible_mask,
)

_ArrayOrVar = TypeVar('_ArrayOrVar', npt.NDArray[Any], Variable)


def _as_interpolation_type(x: _ArrayOrVar) -> _ArrayOrVar:
    if isinstance(x, np.ndarray):
        if x.dtype.kind == 'M':
            return x.astype('int64', copy=False)
    else:
        if x.dtype == DType.datetime64:
            return x - epoch(unit=x.unit)
    return x


def _midpoints(var: Variable, dim: str) -> Variable:
    a = var[dim, :-1]
    b = var[dim, 1:]
    return _as_interpolation_type(a) + 0.5 * (b - a)


def _drop_masked(da: DataArray, dim: str) -> DataArray:
    if (mask := irreducible_mask(da.masks, dim)) is not None:
        return da[~mask]
    return da


@wrap1d(is_partial=True, accept_masks=True)
def interp1d(
    da: DataArray,
    dim: str,
    *,
    kind: int
    | Literal[
        'linear',
        'nearest',
        'nearest-up',
        'zero',
        'slinear',
        'quadratic',
        'cubic',
        'previous',
        'next',
    ] = 'linear',
    fill_value: Any = np.nan,
    **kwargs: Any,
) -> _Interp1dImpl:
    """Interpolate a 1-D function.

    A data array is used to approximate some function f: y = f(x), where y is given by
    the array values and x is is given by the coordinate for the given dimension. This
    class returns a function whose call method uses interpolation to find the value of
    new points.

    The function is a wrapper for scipy.interpolate.interp1d. The differences are:

    - Instead of x and y, a data array defining these is used as input.
    - Instead of an axis, a dimension label defines the interpolation dimension.
    - The returned function does not just return the values of f(x) but a new
      data array with values defined as f(x) and x as a coordinate for the
      interpolation dimension.
    - The returned function accepts an extra argument ``midpoints``. When setting
      ``midpoints=True`` the interpolation uses the midpoints of the new points
      instead of the points itself. The returned data array is then a histogram, i.e.,
      the new coordinate is a bin-edge coordinate.

    If the input data array contains masks that depend on the interpolation dimension
    the masked points are treated as missing, i.e., they are ignored for the definition
    of the interpolation function. If such a mask also depends on additional dimensions
    :py:class:`scipp.DimensionError` is raised since interpolation requires points to
    be 1-D.

    For structured input data dtypes such as vectors, rotations, or linear
    transformations interpolation is structure-element-wise. While this is appropriate
    for vectors, such a naive interpolation for, e.g., rotations does typically not
    yield a rotation so this should be used with care, unless the 'kind' parameter is
    set to, e.g., 'previous', 'next', or 'nearest'.

    Parameters not described above are forwarded to scipy.interpolate.interp1d. The
    most relevant ones are (see :py:class:`scipy.interpolate.interp1d` for details):

    Parameters
    ----------
    da:
        Input data. Defines both dependent and independent variables for interpolation.
    dim:
        Dimension of the interpolation.
    kind:

        - **integer**: order of the spline interpolator
        - **string**:

          - 'zero', 'slinear', 'quadratic', 'cubic':
            spline interpolation of zeroth, first, second or third order
          - 'previous' and 'next':
            simply return the previous or next value of the point
          - 'nearest-up' and 'nearest'
            differ when interpolating half-integers (e.g. 0.5, 1.5) in that
            'nearest-up' rounds up and 'nearest' rounds down
    fill_value:
        Set to 'extrapolate' to allow for extrapolation of points
        outside the range.

    Returns
    -------
    :
        A callable ``f(x)`` that returns interpolated values of ``da`` at ``x``.

    Examples
    --------

    .. plot:: :context: close-figs

      >>> x = sc.linspace(dim='x', start=0.1, stop=1.4, num=4, unit='rad')
      >>> da = sc.DataArray(sc.sin(x), coords={'x': x})

      >>> from scipp.scipy.interpolate import interp1d
      >>> f = interp1d(da, 'x')

      >>> xnew = sc.linspace(dim='x', start=0.1, stop=1.4, num=12, unit='rad')
      >>> f(xnew)  # use interpolation function returned by `interp1d`
      <scipp.DataArray>
      Dimensions: Sizes[x:12, ]
      Coordinates:
      * x                         float64            [rad]  (x)  [0.1, 0.218182, ..., 1.28182, 1.4]
      Data:
                                  float64  [dimensionless]  (x)  [0.0998334, 0.211262, ..., 0.941144, 0.98545]

      >>> f(xnew, midpoints=True)
      <scipp.DataArray>
      Dimensions: Sizes[x:11, ]
      Coordinates:
      * x                         float64            [rad]  (x [bin-edge])  [0.1, 0.218182, ..., 1.28182, 1.4]
      Data:
                                  float64  [dimensionless]  (x)  [0.155548, 0.266977, ..., 0.918992, 0.963297]

    .. plot:: :context: close-figs

      >>> sc.plot({'original':da,
      ...          'interp1d':f(xnew),
      ...          'interp1d-midpoints':f(xnew, midpoints=True)})
    """  # noqa: E501
    import scipy.interpolate as inter

    da = _drop_masked(da, dim)

    def func(xnew: Variable, *, midpoints: bool = False) -> DataArray:
        """Compute interpolation function defined by ``interp1d``
        at interpolation points.

        Parameters
        ----------
        xnew:
            Interpolation points.
        midpoints:
            Interpolate at midpoints of given points.
            The result will be a histogram.
            Default is ``False``.

        Returns
        -------
        :
            Interpolated data array with new coord given by interpolation points
            and data given by interpolation function evaluated at the
            interpolation points (or evaluated at the midpoints of the given points).
        """
        if xnew.unit != da.coords[dim].unit:
            raise UnitError(
                f"Unit of interpolation points '{xnew.unit}' does not match unit "
                f"'{da.coords[dim].unit}' of points defining the interpolation "
                "function along dimension '{dim}'."
            )
        if xnew.dim != dim:
            raise DimensionError(
                f"Dimension of interpolation points '{xnew.dim}' does not match "
                f"interpolation dimension '{dim}'"
            )
        f = inter.interp1d(
            x=_as_interpolation_type(da.coords[dim].values),
            y=da.values,
            kind=kind,
            fill_value=fill_value,
            **kwargs,
        )
        x_ = _as_interpolation_type(_midpoints(xnew, dim) if midpoints else xnew)
        sizes = da.sizes
        sizes[dim] = x_.sizes[dim]
        # ynew is created in this manner to allow for creation of structured dtypes,
        # which is not possible using scipp.array
        ynew = empty(sizes=sizes, unit=da.unit, dtype=da.dtype)
        ynew.values = f(x_.values)
        return DataArray(data=ynew, coords={dim: xnew})

    return func


class _Interp1dImpl(Protocol):
    def __call__(self, xnew: Variable, *, midpoints: bool = False) -> DataArray: ...


__all__ = ['interp1d']
