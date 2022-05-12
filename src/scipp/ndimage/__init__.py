# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for multidimensional image processing.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.ndimage`.
"""
from functools import wraps
from typing import Callable, Union

from ..core import Variable, DataArray
from ..core import CoordError, DimensionError
from ..core import empty_like, islinspace, ones


def _ndfilter(func: Callable) -> Callable:

    @wraps(func)
    def function(x: Union[Variable, DataArray], **kwargs) -> Union[Variable, DataArray]:
        if 'output' in kwargs:
            raise ValueError("The 'output' argument is not supported")
        if x.variances is not None:
            raise ValueError("Filter cannot be applied to input array with variances.")
        if getattr(x, 'masks', None):
            raise ValueError("Filter cannot be applied to input array with masks.")
        return func(x, **kwargs)

    return function


def _delta_to_positional(x: Union[Variable, DataArray], dim, index):
    if isinstance(index, int):
        return index
    coord = x.coords[dim]
    if not islinspace(coord, dim).value:
        raise CoordError(
            f"Data points not regularly spaced along {dim}. To ignore this, "
            "provide a plain value (int or float) instead of a scalar variable. "
            "Note that this will correspond to plain positional indices/offsets.")
    return (len(coord) - 1) * (index.to(unit=coord.unit) / (coord[-1] - coord[0])).value


def _positional_index(x: Union[Variable, DataArray], index, name=None):
    if isinstance(index, (int, Variable)):
        return [_delta_to_positional(x, dim, index) for dim in x.dims]
    if set(index) != set(x.dims):
        raise ValueError(f"Data has dims={x.dims} but input argument '{name}' provides "
                         f"values for {tuple(index)}")
    return [_delta_to_positional(x, dim, index[dim]) for dim in x.dims]


@_ndfilter
def gaussian_filter(x: Union[Variable, DataArray],
                    /,
                    *,
                    sigma,
                    order=0,
                    **kwargs) -> Union[Variable, DataArray]:
    from scipy.ndimage import gaussian_filter
    sigma = _positional_index(x, sigma, name='sigma')
    order = order if isinstance(order, int) else [order[dim] for dim in x.dims]
    out = empty_like(x)
    gaussian_filter(x.values, sigma=sigma, order=order, output=out.values, **kwargs)
    return out


def _make_footprint(x: Union[Variable, DataArray], size, footprint) -> Variable:
    if footprint is None:
        size = _positional_index(x, size, name='size')
        size = [int(s) for s in size]
        footprint = ones(dims=x.dims, shape=size, dtype='bool')
    else:
        if size is not None:
            raise ValueError("Provide either 'size' or 'footprint', not both.")
        if set(footprint.dims) != set(x.dims):
            raise DimensionError(
                f"Dimensions {footprint.dims} must match data dimensions {x.dim}")
    return footprint


def _make_footprint_filter(name):

    def footprint_filter(x: Union[Variable, DataArray],
                         /,
                         *,
                         size=None,
                         footprint=None,
                         origin=0,
                         **kwargs) -> Union[Variable, DataArray]:
        import scipy.ndimage
        footprint = _make_footprint(x, size=size, footprint=footprint)
        origin = _positional_index(x, origin, name='origin')
        origin = [int(s) for s in origin]
        out = empty_like(x)
        scipy_filter = getattr(scipy.ndimage, name)
        scipy_filter(x.values,
                     footprint=footprint.values,
                     origin=origin,
                     output=out.values,
                     **kwargs)
        return out

    footprint_filter.__name__ = name
    footprint_filter.__doc__ = f'Forwards to scipy.ndimage.{name}'
    return _ndfilter(footprint_filter)


generic_filter = _make_footprint_filter('generic_filter')
maximum_filter = _make_footprint_filter('maximum_filter')
median_filter = _make_footprint_filter('median_filter')
minimum_filter = _make_footprint_filter('minimum_filter')
percentile_filter = _make_footprint_filter('percentile_filter')
rank_filter = _make_footprint_filter('rank_filter')

__all__ = [
    'gaussian_filter',
    'generic_filter',
    'maximum_filter',
    'median_filter',
    'minimum_filter',
    'percentile_filter',
    'rank_filter',
]
