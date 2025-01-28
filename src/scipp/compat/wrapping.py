# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock

from collections.abc import Callable, Mapping
from functools import wraps
from typing import Any, TypeVar

from ..core import (
    BinEdgeError,
    DataArray,
    DimensionError,
    Variable,
    VariancesError,
)


def _validated_masks(da: DataArray, dim: str) -> dict[str, Variable]:
    masks = {}
    for name, mask in da.masks.items():
        if dim in mask.dims:
            raise DimensionError(
                f"Cannot apply function along '{dim}' since mask '{name}' depends "
                "on this dimension."
            )
        masks[name] = mask.copy()
    return masks


_Out = TypeVar('_Out', bound=DataArray | Callable[..., DataArray])


def wrap1d(
    is_partial: bool = False, accept_masks: bool = False, keep_coords: bool = False
) -> Callable[[Callable[..., _Out]], Callable[..., _Out]]:
    """Decorator factory for decorating functions that wrap non-scipp 1-D functions.

    1-D functions are typically functions from libraries such as scipy that depend
    on a single 'axis' argument.

    The decorators returned by this factory apply pre- and postprocessing as follows:

    - An 'axis' keyword argument will raise ``ValueError``, recommending use of 'dim'.
      The index of the provided dimension is added as axis to kwargs.
    - Providing data with variances will raise ``sc.VariancesError`` since third-party
      libraries typically cannot handle variances.
    - Coordinates, masks, and attributes that act as "observers", i.e., do not depend
      on the dimension of the function application, are added to the output data array.
      Masks are deep-copied as per the usual requirement in Scipp.

    Parameters
    ----------
    is_partial:
        The wrapped function is partial, i.e., does not return a data
        array itself, but a callable that returns a data array. If true,
        the postprocessing step is not applied to the wrapped function.
        Instead, the callable returned by the decorated function is
        decorated with the postprocessing step.
    accept_masks:
        If false, all masks must apply to the dimension that
        the function is applied to.
    keep_coords:
        If true, preserve the input coordinates.
        If false, drop coordinates that do not apply to the dimension
        the function is applied to.
    """

    def decorator(func: Callable[..., _Out]) -> Callable[..., _Out]:
        @wraps(func)
        def function(da: DataArray, dim: str, **kwargs: Any) -> _Out:
            if 'axis' in kwargs:
                raise ValueError("Use the 'dim' keyword argument instead of 'axis'.")
            if da.variances is not None:
                raise VariancesError(
                    "Cannot apply function to data with uncertainties. If uncertainties"
                    " should be ignored, use 'sc.values(da)' to extract only values."
                )
            if da.sizes[dim] != da.coords[dim].sizes[dim]:
                raise BinEdgeError(
                    "Cannot apply function to data array with bin edges."
                )

            kwargs['axis'] = da.dims.index(dim)
            result = func(da, dim, **kwargs)
            return _postprocess(
                input_da=da,
                output_da=result,
                dim=dim,
                is_partial=is_partial,
                accept_masks=accept_masks,
                keep_coords=keep_coords,
            )

        return function

    return decorator


def _postprocess(
    *,
    input_da: DataArray,
    output_da: _Out,
    dim: str,
    is_partial: bool,
    accept_masks: bool,
    keep_coords: bool,
) -> _Out:
    if accept_masks:
        masks = _remove_columns_in_dim(input_da.masks, dim)
    else:
        masks = _validated_masks(input_da, dim)
    if keep_coords:
        coords: Mapping[str, Variable] = input_da.coords
    else:
        coords = _remove_columns_in_dim(input_da.coords, dim)

    def add_observing_metadata(da: DataArray) -> DataArray:
        # operates in-place!
        da.coords.update(coords)
        da.masks.update((key, mask.copy()) for key, mask in masks.items())
        return da

    if is_partial:  # corresponds to `not isinstance(out_da, DataArray)`

        def postprocessing(func: Callable[..., DataArray]) -> Callable[..., DataArray]:
            @wraps(func)
            def function(*args: Any, **kwargs: Any) -> DataArray:
                return add_observing_metadata(func(*args, **kwargs))

            return function

        return postprocessing(output_da)  # type: ignore[arg-type, return-value]
    else:
        return add_observing_metadata(output_da)  # type: ignore[arg-type, return-value]


def _remove_columns_in_dim(
    mapping: Mapping[str, Variable], dim: str
) -> dict[str, Variable]:
    return {key: var for key, var in mapping.items() if dim not in var.dims}
