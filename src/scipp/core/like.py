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
    If input is a :class:`Variable`, it constructs a :class:`Variable` with the same
    dims, shape, unit and dtype, but with all values initialized to 0. If the input
    has variances, all variances in the output are set to 0.

    If input is a :class:`DataArray`, it constructs a :class:`DataArray` with the same
    coordinates, attributes and masks as the input, but sets the data values to 1.
    If the input data has variances, all variances in the output are set to 0.
    Note that coordinates and attributes are shallow-copied, while masks are
    deep-copied.

    :param var: Input variable or data array.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones_like`
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
    If input is a :class:`Variable`, it constructs a :class:`Variable` with the same
    dims, shape, unit and dtype, but with all values initialized to 1. If the input
    has variances, all variances in the output are set to 1.

    If input is a :class:`DataArray`, it constructs a :class:`DataArray` with the same
    coordinates, attributes and masks as the input, but sets the data values to 1.
    If the input data has variances, all variances in the output are set to 1.
    Note that coordinates and attributes are shallow-copied, while masks are
    deep-copied.

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
    If input is a :class:`Variable`, it constructs a :class:`Variable` with the same
    dims, shape, unit and dtype as the input variable, but with uninitialized values.
    If the input has variances, all variances in the output exist but are uninitialized.

    If input is a :class:`DataArray`, it constructs a :class:`DataArray` with the same
    coordinates, attributes and masks as the input, but replaces the data with a
    :class:`Variable` containing uninitialized values. If the input data has variances,
    all variances in the output exist but are uninitialized.
    Note that coordinates and attributes are shallow-copied, while masks are
    deep-copied.

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
    If input is a :class:`Variable`, it constructs a :class:`Variable` with values
    initialized to the specified value with dimensions labels and shape provided by an
    existing variable.

    If input is a :class:`DataArray`, it constructs a :class:`DataArray` with the same
    coordinates, attributes and masks as the input, but replaces the data with a
    :class:`Variable` containing values initialized to the specified value.
    Note that coordinates and attributes are shallow-copied, while masks are
    deep-copied.

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
