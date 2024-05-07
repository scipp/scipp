# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for multidimensional image processing.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.ndimage`.
"""

from collections.abc import Callable, Mapping
from functools import wraps
from typing import Any, Protocol, TypeVar

import scipy.ndimage

from ...core import (
    CoordError,
    DataArray,
    DimensionError,
    Variable,
    VariancesError,
    empty_like,
    islinspace,
    ones,
)
from ...typing import VariableLike

_T = TypeVar('_T', Variable, DataArray)


def _ndfilter(
    func: Callable[..., _T],
) -> Callable[..., _T]:
    @wraps(func)
    def function(x: _T, **kwargs: Any) -> _T:
        if 'output' in kwargs:
            raise TypeError("The 'output' argument is not supported")
        if x.variances is not None:
            raise VariancesError(
                "Filter cannot be applied to input array with variances."
            )
        if getattr(x, 'masks', None):
            raise ValueError("Filter cannot be applied to input array with masks.")
        return func(x, **kwargs)

    return function


def _delta_to_positional(
    x: Any,
    dim: str,
    index: float | Variable | Mapping[str, float | Variable],
    dtype: type,
) -> Any:
    if not isinstance(index, Variable):
        return index
    coord = x.coords[dim]
    if not islinspace(coord, dim).value:
        raise CoordError(
            f"Data points not regularly spaced along {dim}. To ignore this, "
            f"provide a plain value (convertible to {dtype.__name__}) instead of a "
            "scalar variable. Note that this will correspond to plain positional "
            "indices/offsets."
        )
    pos = (len(coord) - 1) * (index.to(unit=coord.unit) / (coord[-1] - coord[0])).value
    return dtype(pos)


def _require_matching_dims(
    index: Mapping[str, float | Variable],
    x: VariableLike,
    name: str | None,
) -> None:
    if set(index) != set(x.dims):
        raise KeyError(
            f"Data has dims={x.dims} but input argument '{name}' provides "
            f"values for {tuple(index)}"
        )


def _positional_index(
    x: Any,
    index: float | Variable | Mapping[str, float | Variable],
    name: str | None = None,
    dtype: type = int,
) -> list[Any]:
    if not isinstance(index, Mapping):
        return [_delta_to_positional(x, dim, index, dtype=dtype) for dim in x.dims]
    _require_matching_dims(index, x, name)
    return [_delta_to_positional(x, dim, index[dim], dtype=dtype) for dim in x.dims]


@_ndfilter
def gaussian_filter(
    x: _T,
    /,
    *,
    sigma: float | Variable | Mapping[str, float | Variable],
    order: int | Mapping[str, int] | None = 0,
    **kwargs: Any,
) -> _T:
    """
    Multidimensional Gaussian filter.

    This is a wrapper around :py:func:`scipy.ndimage.gaussian_filter`. See there for
    full argument description. There are two key differences:

    - This wrapper uses explicit dimension labels in the ``sigma`` and ``order``
      arguments. For example, instead of ``sigma=[4, 6]`` use
      ``sigma={'time':4, 'space':6}``
      (with appropriate dimension labels for the data).
    - Coordinate values can be used (and should be preferred) for ``sigma``. For
      example, instead of ``sigma=[4, 6]`` use
      ``sigma={'time':sc.scalar(5.0, unit='ms'), 'space':sc.scalar(1.2, unit='mm')}``.
      In this case it is required that the corresponding coordinates exist and form a
      "linspace", i.e., are evenly spaced.

    Warning
    -------
    If ``sigma`` is an integer or a mapping to integers then coordinate values are
    ignored. That is, the filter is applied even if the data points are not evenly
    spaced. The resulting filtered data may thus have no meaningful interpretation.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input variable or data array.
    sigma:
        Standard deviation for Gaussian kernel. The standard deviations of the Gaussian
        filter are given as a mapping from dimension labels to numbers or scalar
        variables, or as a single number or scalar variable, in which case it is equal
        for all axes.
    order:
        The order of the filter along each dimension, given as mapping from dimension
        labels to integers, or as a single integer. An order of 0 corresponds to
        convolution with a Gaussian kernel. A positive order corresponds to convolution
        with that derivative of a Gaussian.

    Returns
    -------
    : scipp.typing.VariableLike
        Filtered variable or data array

    Examples
    --------

    .. plot:: :context: close-figs

      >>> from scipp.scipy.ndimage import gaussian_filter
      >>> da = sc.data.data_xy()
      >>> da.plot()

    With sigma as integer:

    .. plot:: :context: close-figs

      >>> filtered = gaussian_filter(da, sigma=4)
      >>> filtered.plot()

    With sigma based on input coordinate values:

    .. plot:: :context: close-figs

      >>> filtered = gaussian_filter(da, sigma=sc.scalar(0.1, unit='mm'))
      >>> filtered.plot()

    With different sigma for different dimensions:

    .. plot:: :context: close-figs

      >>> filtered = gaussian_filter(da, sigma={'x':sc.scalar(0.01, unit='mm'),
      ...                                       'y':sc.scalar(1.0, unit='mm')})
      >>> filtered.plot()
    """
    sigma_values = _positional_index(x, sigma, name='sigma', dtype=float)
    if isinstance(order, Mapping):
        _require_matching_dims(order, x, 'order')
        order = [order[dim] for dim in x.dims]  # type: ignore[assignment]
    out = empty_like(x)
    scipy.ndimage.gaussian_filter(
        x.values, sigma=sigma_values, order=order, output=out.values, **kwargs
    )
    return out


def _make_footprint(
    x: Variable | DataArray,
    size: int | Variable | Mapping[str, int | Variable] | None,
    footprint: Variable | None,
) -> Variable:
    if footprint is None:
        if size is None:
            raise ValueError("Provide either 'footprint' or 'size'.")
        footprint = ones(
            dims=x.dims, shape=_positional_index(x, size, name='size'), dtype='bool'
        )
    else:
        if size is not None:
            raise ValueError("Provide either 'size' or 'footprint', not both.")
        if set(footprint.dims) != set(x.dims):
            raise DimensionError(
                f"Dimensions {footprint.dims} must match data dimensions {x.dim}"
            )
    return footprint


class _FootprintFilter(Protocol):
    def __call__(
        self,
        x: _T,
        /,
        *,
        size: int | Variable | Mapping[str, int | Variable] | None = None,
        footprint: Variable | None = None,
        origin: int | Variable | Mapping[str, int | Variable] | None = 0,
        **kwargs: Any,
    ) -> _T: ...


def _make_footprint_filter(
    name: str, example: bool = True, extra_args: str = ''
) -> _FootprintFilter:
    def footprint_filter(
        x: _T,
        /,
        *,
        size: int | Variable | Mapping[str, int | Variable] | None = None,
        footprint: Variable | None = None,
        origin: int | Variable | Mapping[str, int | Variable] = 0,
        **kwargs: Any,
    ) -> _T:
        footprint = _make_footprint(x, size=size, footprint=footprint)
        out = empty_like(x)
        scipy_filter = getattr(scipy.ndimage, name)
        scipy_filter(
            x.values,
            footprint=footprint.values,
            origin=_positional_index(x, origin, name='origin'),
            output=out.values,
            **kwargs,
        )
        return out

    footprint_filter.__name__ = name
    if extra_args:
        extra_args = f', {extra_args}'
    doc = f"""
    Calculate a multidimensional {name.replace('_', ' ')}.

    This is a wrapper around :py:func:`scipy.ndimage.{name}`. See there for full
    argument description. There are two key differences:

    - This wrapper uses explicit dimension labels in the ``size``, ``footprint``, and
      ``origin`` arguments. For example, instead of ``size=[4, 6]`` use
      ``size={{'time':4, 'space':6}}`` (with appropriate dimension labels for the data).
    - Coordinate values can be used (and should be preferred) for ``size`` and
      ``origin``. For example, instead of ``size=[4, 6]`` use
      ``size={{'time':sc.scalar(5.0, unit='ms'), 'space':sc.scalar(1.2, unit='mm')}}``.
      In this case it is required that the corresponding coordinates exist and form a
      "linspace", i.e., are evenly spaced.

    Warning
    -------
    When ``size`` is an integer or a mapping to integers or when ``footprint`` is
    given, coordinate values are ignored. That is, the filter is applied even if the
    data points are not evenly spaced. The resulting filtered data may thus have no
    meaningful interpretation.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input variable or data array.
    size:
        Integer or scalar variable or mapping from dimension labels to integers or
        scalar variables. Defines the footprint (see below).
    footprint:
        Variable with same dimension labels (but different shape) as the input data.
        The boolean values specify (implicitly) a shape, but also which of the elements
        within this shape will get passed to the filter function.
    origin:
        Integer or scalar variable or mapping from dimension labels to integers or
        scalar variables. Controls the placement of the filter on the input array.

    Returns
    -------
    : scipp.typing.VariableLike
        Filtered variable or data array
    """
    if example:
        doc += f"""
    Examples
    --------

    .. plot:: :context: close-figs

      >>> from scipp.scipy.ndimage import {name}
      >>> da = sc.data.data_xy()
      >>> da.plot()

    With size as integer:

    .. plot:: :context: close-figs

      >>> filtered = {name}(da, size=4{extra_args})
      >>> filtered.plot()

    With size based on input coordinate values:

    .. plot:: :context: close-figs

      >>> filtered = {name}(da, size=sc.scalar(0.2, unit='mm'){extra_args})
      >>> filtered.plot()

    With different size for different dimensions:

    .. plot:: :context: close-figs

      >>> filtered = {name}(da, size={{'x':sc.scalar(0.2, unit='mm'),
      ...                       {' ' * len(name)}'y':sc.scalar(1.1, unit='mm')}}{extra_args})
      >>> filtered.plot()
    """  # noqa: E501
    footprint_filter.__doc__ = doc
    return _ndfilter(footprint_filter)


generic_filter = _make_footprint_filter('generic_filter', example=False)
maximum_filter = _make_footprint_filter('maximum_filter')
median_filter = _make_footprint_filter('median_filter')
minimum_filter = _make_footprint_filter('minimum_filter')
percentile_filter = _make_footprint_filter(
    'percentile_filter', extra_args='percentile=80'
)
rank_filter = _make_footprint_filter('rank_filter', extra_args='rank=3')

__all__ = [
    'gaussian_filter',
    'generic_filter',
    'maximum_filter',
    'median_filter',
    'minimum_filter',
    'percentile_filter',
    'rank_filter',
]
