# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from typing import Any
from .._scipp import core as _cpp
from .variable import ones, zeros, empty, full
from ..typing import VariableLikeType


def _to_variable_or_data_array(var, new_values):
    if isinstance(var, _cpp.DataArray):
        return _cpp.DataArray(data=new_values,
                              coords={c: coord
                                      for c, coord in var.coords.items()},
                              attrs={a: attr
                                     for a, attr in var.attrs.items()},
                              masks={m: mask.copy()
                                     for m, mask in var.masks.items()})
    else:
        return new_values


def zeros_like(obj: VariableLikeType, /) -> VariableLikeType:
    """Return a new object with the same dims, shape, unit,
    and dtype as the input and all elements initialized to 0.

    If the input has variances, all variances in the output are set to 0.
    If the input is a :class:`DataArray`, coordinates and attributes are shallow-copied
    and masks are deep-copied.

    Parameters
    ----------
    obj: scipp.Variable or scipp.DataArray
        Input object defining dims, shape, unit, and dtype of the output.

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
    new_values = zeros(dims=obj.dims,
                       shape=obj.shape,
                       unit=obj.unit,
                       dtype=obj.dtype,
                       with_variances=obj.variances is not None)
    return _to_variable_or_data_array(obj, new_values)


def ones_like(obj: VariableLikeType, /) -> VariableLikeType:
    """Return a new object with the same dims, shape, unit,
    and dtype as the input and all elements initialized to 1.

    If the input has variances, all variances in the output are set to 1.
    If the input is a :class:`DataArray`, coordinates and attributes are shallow-copied
    and masks are deep-copied.

    Parameters
    ----------
    obj: scipp.Variable or scipp.DataArray
        Input object defining dims, shape, unit, and dtype of the output.

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
    new_values = ones(dims=obj.dims,
                      shape=obj.shape,
                      unit=obj.unit,
                      dtype=obj.dtype,
                      with_variances=obj.variances is not None)
    return _to_variable_or_data_array(obj, new_values)


def empty_like(obj: VariableLikeType, /) -> VariableLikeType:
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
    obj: scipp.Variable or scipp.DataArray
        Input object defining dims, shape, unit, and dtype of the output

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
    new_values = empty(dims=obj.dims,
                       shape=obj.shape,
                       unit=obj.unit,
                       dtype=obj.dtype,
                       with_variances=obj.variances is not None)
    return _to_variable_or_data_array(obj, new_values)


def full_like(obj: VariableLikeType,
              /,
              value: Any,
              *,
              variance: Any = None) -> VariableLikeType:
    """Return a new object with the same dims, shape, unit,
    and dtype as the input and all elements initialized to the given value.

    If the input is a :class:`DataArray`, coordinates and attributes are shallow-copied
    and masks are deep-copied.

    Parameters
    ----------
    obj: scipp.Variable or scipp.DataArray
        Input object defining dims, shape, unit, and dtype of the output
    value:
        The value to fill the data with.
    variance:
        The variance to fill the Variable with. If None
        or not provided, the variances will not be set.

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
    new_values = full(dims=obj.dims,
                      shape=obj.shape,
                      unit=obj.unit,
                      dtype=obj.dtype,
                      value=value,
                      variance=variance)
    return _to_variable_or_data_array(obj, new_values)
