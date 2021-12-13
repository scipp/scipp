# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from .. import DataArray, DimensionError, VariancesError

from functools import wraps
from typing import Callable, Union


def _validated_masks(da, dim):
    masks = {}
    for name, mask in da.masks.items():
        if dim in mask.dims:
            raise DimensionError(
                f"Cannot apply function along '{dim}' since mask '{name}' depends"
                "on this dimension.")
        masks[name] = mask.copy()
    return masks


def wrap1d(is_partial=False):
    """Decorator factory for decorating function that wrap non-scipp 1-D functions.

    1-D functions are typically functions from libraries such as scipy that depend
    on a single 'axis' argument.

    The decorators returned by this factory apply pre- and postprcoessing as follows:

    - An 'axis' keyword argument will raise ``ValueError``, recommending use of 'dim'.
    - Providing data with variances will raise ``sc.VariancesError`` since third-party
      libraries typically cannot handle variances.
    - Coordinates, masks, and attributes that act as "observers", i.e., do not depend
      on the dimension of the function application or added do the output data array.
      Masks are deep-copied as per the usual requirement in scipp.

    :param is_partial: The wrapped function is partial, i.e., does not return a data
                       array itself, but a callable that returns a data array. If true,
                       the posprocessing step is not applied to the wrapped function.
                       Instead the callable returned by the decorated function is
                       decorated with the postprocessing step.
    """
    def decorator(func: Callable) -> Callable:
        @wraps(func)
        def function(da: DataArray, dim: str, **kwargs) -> Union[DataArray, Callable]:
            if 'axis' in kwargs:
                raise ValueError("Use the 'dim' keyword argument instead of 'axis'.")
            if da.variances is not None:
                raise VariancesError(
                    "Cannot apply function to data with uncertainties. If uncertainties"
                    " should be ignored, use 'sc.values(da)' to extract only values.")

            coords = {k: v for k, v in da.coords.items() if dim not in v.dims}
            masks = _validated_masks(da, dim)
            attrs = {k: v for k, v in da.attrs.items() if dim not in v.dims}

            def postprocess(da):
                for k, v in coords.items():
                    da.coords[k] = v
                for k, v in masks.items():
                    da.masks[k] = v.copy()
                for k, v in attrs.items():
                    da.attrs[k] = v
                return da

            def postprocessing_decorator(func):
                @wraps(func)
                def function(*args, **kwargs):
                    return postprocess(func(*args, **kwargs))

                return function

            if is_partial:
                return postprocessing_decorator(func(da, dim, **kwargs))
            else:
                return postprocess(func(da, dim, **kwargs))

        return function

    return decorator
