# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for multidimensional image processing.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.ndimage`.
"""

from functools import wraps
from typing import Callable, Optional, Union, Any, overload, TypeVar

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
from ...typing import VariableLike, VariableLikeType


def _ndfilter(
    func: Callable[..., Union[Variable, DataArray]],
) -> Callable[..., Union[Variable, DataArray]]:
    @wraps(func)
    def function(
        x: Union[Variable, DataArray], **kwargs: Any
    ) -> Union[Variable, DataArray]:
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


_I = TypeVar('_I', bound=Union[int, dict[str, Union[int, Variable]]])
_T = TypeVar('_T')


@overload
def _delta_to_positional(
    x: DataArray,
    dim: str,
    index: Variable,
    dtype: type[_T],
) -> _T: ...


@overload
def _delta_to_positional(
    x: Any,
    dim: str,
    index: _I,
    dtype: type[_T],
) -> _I: ...


def _delta_to_positional(
    x: Any,
    dim: str,
    index: Union[int, Variable, dict[str, Union[int, Variable]]],
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
    index: dict[str, Union[int, Variable]],
    x: VariableLike,
    name: Optional[str],
) -> None:
    if set(index) != set(x.dims):
        raise KeyError(
            f"Data has dims={x.dims} but input argument '{name}' provides "
            f"values for {tuple(index)}"
        )


@overload
def _positional_index(
    x: DataArray,
    index: Variable,
    name: Optional[str],
    dtype: type[_T],
) -> list[_T]: ...


@overload
def _positional_index(
    x: Union[Variable, DataArray],
    index: _I,
    name: Optional[str],
    dtype: type[_T],
) -> list[_I]: ...


def _positional_index(
    x: Union[Variable, DataArray],
    index: Union[int, Variable, dict[str, Union[int, Variable]]],
    name: Optional[str] = None,
    dtype: type = int,
):
    if not isinstance(index, dict):
        return [_delta_to_positional(x, dim, index, dtype=dtype) for dim in x.dims]
    _require_matching_dims(index, x, name)
    return [_delta_to_positional(x, dim, index[dim], dtype=dtype) for dim in x.dims]


@_ndfilter
def gaussian_filter(
    x: VariableLikeType,
    /,
    *,
    sigma: Union[float, Variable, dict[str, Union[int, float, Variable]]],
    order: Optional[Union[int, dict[str, int]]] = 0,
    **kwargs: Any,
) -> VariableLikeType:
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
    sigma = _positional_index(x, sigma, name='sigma', dtype=float)
    if isinstance(order, dict):
        _require_matching_dims(order, x, 'order')
        order = [order[dim] for dim in x.dims]
    out = empty_like(x)
    scipy.ndimage.gaussian_filter(
        x.values, sigma=sigma, order=order, output=out.values, **kwargs
    )
    return out


def _make_footprint(x: Union[Variable, DataArray], size, footprint) -> Variable:
    if footprint is None:
        size = _positional_index(x, size, name='size')
        footprint = ones(dims=x.dims, shape=size, dtype='bool')
    else:
        if size is not None:
            raise ValueError("Provide either 'size' or 'footprint', not both.")
        if set(footprint.dims) != set(x.dims):
            raise DimensionError(
                f"Dimensions {footprint.dims} must match data dimensions {x.dim}"
            )
    return footprint


def _make_footprint_filter(name: str, example: bool = True, extra_args: str = ''):
    def footprint_filter(
        x: VariableLikeType,
        /,
        *,
        size: Optional[Union[int, Variable, dict[str, Union[int, Variable]]]] = None,
        footprint: Optional[Variable] = None,
        origin: Optional[Union[int, Variable, dict[str, Union[int, Variable]]]] = 0,
        **kwargs: Any,
    ) -> VariableLikeType:
        footprint = _make_footprint(x, size=size, footprint=footprint)
        origin = _positional_index(x, origin, name='origin')
        out = empty_like(x)
        scipy_filter = getattr(scipy.ndimage, name)
        scipy_filter(
            x.values,
            footprint=footprint.values,
            origin=origin,
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
