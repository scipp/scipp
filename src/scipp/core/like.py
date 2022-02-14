# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from typing import Any as _Any, Union as _Union
from .._scipp import core as _cpp
from .variable import ones, zeros, empty, full


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


def zeros_like(
    var: _Union[_cpp.Variable,
                _cpp.DataArray]) -> _Union[_cpp.Variable, _cpp.DataArray]:
    """
    Return a Variable or DataArray with the same dims, shape, unit, and dtype as the
    input and all values initialized to 0.

    If the input has variances, all variances in the output are set to 0.
    If the input is a data array, coordinates and attributes are shallow-copied
    and masks are deep copied.

    Parameters
    ----------
    var:
        Input object defining dims, shape, unit, and dtype of the output

    Returns
    -------
    :
        New object of zeros.

    See Also
    --------
    zeros: Create zeros but based on given dims and shape
    ones_like : Create an object initialized with ones
    """
    new_values = zeros(dims=var.dims,
                       shape=var.shape,
                       unit=var.unit,
                       dtype=var.dtype,
                       with_variances=var.variances is not None)
    return _to_variable_or_data_array(var, new_values)


def ones_like(
    var: _Union[_cpp.Variable,
                _cpp.DataArray]) -> _Union[_cpp.Variable, _cpp.DataArray]:
    """
    Constructs a new object with the same dims, shape, unit and dtype as the input
    (:class:`Variable` or :class:`DataArray`), but with all values initialized to 1.
    If the input has variances, all variances in the output are set to 1.
    If the input is a :class:`DataArray`, coordinates and attributes are shallow-copied
    and masks are deep copied.

    :param var: Input variable or data array.

    :seealso: :py:func:`scipp.ones` :py:func:`scipp.zeros_like`
    """
    new_values = ones(dims=var.dims,
                      shape=var.shape,
                      unit=var.unit,
                      dtype=var.dtype,
                      with_variances=var.variances is not None)
    return _to_variable_or_data_array(var, new_values)


def empty_like(
    var: _Union[_cpp.Variable,
                _cpp.DataArray]) -> _Union[_cpp.Variable, _cpp.DataArray]:
    """
    Constructs a new object with the same dims, shape, unit and dtype as the input
    (:class:`Variable` or :class:`DataArray`), but with all values uninitialized.
    If the input has variances, all variances in the output exist but are uninitialized.
    If the input is a :class:`DataArray`, coordinates and attributes are shallow-copied
    and masks are deep copied.

    :param var: Input variable or data array.

    :seealso: :py:func:`scipp.empty` :py:func:`scipp.zeros_like`
              :py:func:`scipp.ones_like`
    """
    new_values = empty(dims=var.dims,
                       shape=var.shape,
                       unit=var.unit,
                       dtype=var.dtype,
                       with_variances=var.variances is not None)
    return _to_variable_or_data_array(var, new_values)


def full_like(var: _cpp.Variable, value: _Any, variance: _Any = None) -> _cpp.Variable:
    """
    Constructs a new object with the same dims, shape, unit and dtype as the input
    (:class:`Variable` or :class:`DataArray`), but with all values, and optionally
    variances, initialized to the specified ``value`` and ``variance``.
    If the input is a :class:`DataArray`, coordinates and attributes are shallow-copied
    and masks are deep copied.

    :param var: Input variable or data array.
    :param value: The value to fill the data with.
    :param variance: Optional, the variance to fill the Variable with. If None
        or not provided, the variances will not be set.

    :seealso: :py:func:`scipp.zeros_like` :py:func:`scipp.ones_like`
              :py:func:`scipp.empty_like`
    """
    new_values = full(dims=var.dims,
                      shape=var.shape,
                      unit=var.unit,
                      dtype=var.dtype,
                      value=value,
                      variance=variance)
    return _to_variable_or_data_array(var, new_values)
