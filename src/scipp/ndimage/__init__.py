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
from ..core import CoordError
from ..core import empty_like, islinspace


def ndfilter(func: Callable) -> Callable:

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
    if isinstance(index, Variable):
        return [_delta_to_positional(x, dim, index) for dim in x.dims]
    if set(index) != set(x.dims):
        raise ValueError(f"Data has dims={x.dims} but input argument '{name}' provides "
                         f"values for {tuple(index)}")
    return [_delta_to_positional(x, dim, index[dim]) for dim in x.dims]


@ndfilter
def gaussian_filter(x: Union[Variable, DataArray], /, *, sigma,
                    **kwargs) -> Union[Variable, DataArray]:
    from scipy.ndimage import gaussian_filter
    out = empty_like(x)

    gaussian_filter(x.values,
                    sigma=_positional_index(x, sigma, name='sigma'),
                    output=out.values,
                    **kwargs)
    return out


__all__ = ['gaussian_filter']
