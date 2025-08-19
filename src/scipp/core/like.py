# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from typing import Any, TypeVar

from .concepts import rewrap_output_data
from .cpp_classes import DataArray, Variable
from .variable import empty, full, ones, zeros

_T = TypeVar('_T', Variable, DataArray)


def _init_args_from_obj_and_kwargs(obj: _T, kwargs: dict[str, Any]) -> dict[str, Any]:
    default: dict[str, Any] = {
        'unit': obj.unit,
        'dtype': obj.dtype,
    }
    if 'sizes' not in kwargs:
        default.setdefault('dims', obj.dims)
        default.setdefault('shape', obj.shape)
    return default | kwargs


def zeros_like(obj: _T, **kwargs: Any) -> _T:
    """Return a new object with the same dims, shape, unit,
    and dtype as the input and all elements initialized to 0.

    If the input has variances, all variances in the output are set to 0.
    If the input is a :class:`DataArray`, coordinates and attributes are shallow-copied
    and masks are deep-copied.

    Parameters
    ----------
    obj: scipp.Variable | scipp.DataArray
        Input object defining dims, shape, unit, and dtype of the output.
    kwargs:
        Override arguments passed to :func:`scipp.zeros`.

    Returns
    -------
    : Same type as input
        New object of zeros.

    See Also
    --------
    scipp.zeros:
        Create zeros but based on given dims and shape.
    scipp.ones_like:
        Create an object initialized with ones.
    scipp.full_like:
        Create an object filled with a given value.
    scipp.empty_like:
        Create an object with uninitialized elements.
    """
    kw = _init_args_from_obj_and_kwargs(obj, kwargs)
    kw.setdefault('with_variances', obj.variances is not None)
    new_values = zeros(**kw)
    return rewrap_output_data(obj, new_values)


def ones_like(obj: _T, **kwargs: Any) -> _T:
    """Return a new object with the same dims, shape, unit,
    and dtype as the input and all elements initialized to 1.

    If the input has variances, all variances in the output are set to 1.
    If the input is a :class:`DataArray`, coordinates and attributes are shallow-copied
    and masks are deep-copied.

    Parameters
    ----------
    obj: scipp.Variable | scipp.DataArray
        Input object defining dims, shape, unit, and dtype of the output.
    kwargs:
        Override arguments passed to ``scipp.ones``.

    Returns
    -------
    : Same type as input
        New object of ones.

    See Also
    --------
    scipp.ones:
        Create ones but based on given dims and shape.
    scipp.zeros_like:
        Create an object initialized with zeros.
    scipp.full_like:
        Create an object filled with a given value.
    scipp.empty_like:
        Create an object with uninitialized elements.
    """
    kw = _init_args_from_obj_and_kwargs(obj, kwargs)
    kw.setdefault('with_variances', obj.variances is not None)
    new_values = ones(**kw)
    return rewrap_output_data(obj, new_values)


def empty_like(obj: _T, **kwargs: Any) -> _T:
    """Return a new object with the same dims, shape, unit,
    and dtype as the input and all elements uninitialized.

    If the input has variances, all variances in the output exist but are uninitialized.
    If the input is a :class:`DataArray`, coordinates and attributes are shallow-copied
    and masks are deep-copied.

    Warning
    -------
    Reading from any elements before writing to them produces undefined results.

    Parameters
    ----------
    obj: scipp.Variable | scipp.DataArray
        Input object defining dims, shape, unit, and dtype of the output
    kwargs:
        Override arguments passed to ``scipp.empty``.

    Returns
    -------
    : Same type as input
        New object with uninitialized values and maybe variances.

    See Also
    --------
    scipp.empty:
        Create an uninitialized object based on given dims and shape.
    scipp.zeros_like:
        Create an object initialized with zeros.
    scipp.ones_like:
        Create an object initialized with ones.
    scipp.full_like:
        Create an object filled with a given value.
    """
    kw = _init_args_from_obj_and_kwargs(obj, kwargs)
    kw.setdefault('with_variances', obj.variances is not None)
    new_values = empty(**kw)
    return rewrap_output_data(obj, new_values)


def full_like(obj: _T, /, value: Any, *, variance: Any = None, **kwargs: Any) -> _T:
    """Return a new object with the same dims, shape, unit,
    and dtype as the input and all elements initialized to the given value.

    If the input is a :class:`DataArray`, coordinates and attributes are shallow-copied
    and masks are deep-copied.

    Parameters
    ----------
    obj: scipp.Variable | scipp.DataArray
        Input object defining dims, shape, unit, and dtype of the output
    value:
        The value to fill the data with.
    variance:
        The variance to fill the Variable with. If None
        or not provided, the variances will not be set.
    kwargs:
        Override arguments passed to ``scipp.full``.

    Returns
    -------
    : Same type as input
        New object with elements set to given values and variances.

    See Also
    --------
    scipp.full:
        Create an object filled with given value based on given dims and shape.
    scipp.zeros_like:
        Create an object initialized with zeros.
    scipp.ones_like:
        Create an object initialized with ones.
    scipp.empty_like:
        Create an object with uninitialized elements.
    """
    kw = _init_args_from_obj_and_kwargs(obj, kwargs)
    kw['value'] = value
    kw['variance'] = variance
    new_values = full(**kw)
    return rewrap_output_data(obj, new_values)
