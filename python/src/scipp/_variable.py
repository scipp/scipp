# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from typing import Any as _Any, Sequence as _Sequence, Union as _Union
import numpy as _np


def filter(x, key):
    """
    Selects elements for a Variable using a filter (mask).

    The filter variable must be 1D and of bool type.
    A true value in the filter means the corresponding element in the input is
    selected and will be copied to the output.
    A false value in the filter discards the corresponding element
    in the input.

    :param x: Variable to filter.
    :param key: Variable which defines the filter.
    :type x: Variable
    :type key: Variable
    :raises: If the filter variable is not 1 dimensional.
    :returns: New variable containing the data selected by the filter.
    :rtype: Variable
    """
    return _call_cpp_func(_cpp.filter, x, key)


def split(x, dim, inds):
    """
    Split a Variable along a given Dimension.

    :param x: Variable to split.
    :param dim: Dimension along which to perform the split.
    :param inds: List of indices  where the variable will split.
    :type x: Variable
    :type dim: str
    :type inds: list
    :returns: A list of variables.
    :rtype: list
    """
    return _call_cpp_func(_cpp.split, x, dim, inds)


def islinspace(x):
    """
    Check if the values of a variable are evenly spaced.

    :param x: Variable to check.
    :type x: Variable
    :returns: True if the variable contains regularly spaced values,
      False otherwise.
    :rtype: bool
    """
    return _call_cpp_func(_cpp.islinspace, x)


def scalar(value: _Any,
           variance: _Any = None,
           unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
           dtype: type(_cpp.dtype.float64) = None) -> _cpp.Variable:
    """Constructs a zero dimensional :class:`Variable` with a unit and optional
    variance.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones`
              :py:func:`scipp.empty` :py:func:`scipp.array`

    :param value: Initial value.
    :param variance: Optional, initial variance, Default=None
    :param unit: Optional, unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
      Cannot be specified for value types of
      str, Dataset or DataArray.
    :returns: A scalar (zero-dimensional) Variable.
    :rtype: Variable
    """
    if dtype is None:
        return _cpp.Variable(value=value, variance=variance, unit=unit)
    else:
        try:
            return _cpp.Variable(value=value,
                                 variance=variance,
                                 unit=unit,
                                 dtype=dtype)
        except TypeError:
            # Raise a more comprehensible error message in the case
            # where a dtype cannot be specified.
            raise TypeError(f"Cannot convert {value} to {dtype}. "
                            f"Try omitting the 'dtype=' parameter.")


def zeros(*,
          dims: _Sequence[str],
          shape: _Sequence[int],
          unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
          dtype: type(_cpp.dtype.float64) = _cpp.dtype.float64,
          variances: bool = False) -> _cpp.Variable:
    """Constructs a :class:`Variable` with default initialized values with
    given dimension labels and shape.
    Optionally can add default initialized variances.
    Only keyword arguments accepted.

    :seealso: :py:func:`scipp.ones` :py:func:`scipp.empty`
              :py:func:`scipp.scalar` :py:func:`scipp.array`

    :param dims: Dimension labels.
    :param shape: Dimension sizes.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param variances: Optional, boolean flag, if True includes variances
      initialized to the default value for dtype.
      For example for a float type values and variances would all be
      initialized to 0.0. Default=False
    """
    return _cpp.Variable(dims=dims,
                         shape=shape,
                         unit=unit,
                         dtype=dtype,
                         variances=variances)


def zeros_like(var: _cpp.Variable) -> _cpp.Variable:
    """Constructs a :class:`Variable` with the same dims, shape, unit and dtype
    as the input variable, but with all values initialized to 0. If the input
    has variances, all variances in the output are set to 0.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones_like`

    :param var: Input variable.
    """
    return zeros(dims=var.dims,
                 shape=var.shape,
                 unit=var.unit,
                 dtype=var.dtype,
                 variances=var.variances is not None)


def ones(*,
         dims: _Sequence[str],
         shape: _Sequence[int],
         unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
         dtype: type(_cpp.dtype.float64) = _cpp.dtype.float64,
         variances: bool = False) -> _cpp.Variable:
    """Constructs a :class:`Variable` with values initialized to 1 with
    given dimension labels and shape.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.empty`
              :py:func:`scipp.scalar` :py:func:`scipp.array`

    :param dims: Dimension labels.
    :param shape: Dimension sizes.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param variances: Optional, boolean flag, if True includes variances
                      initialized to 1. Default=False
    """
    return _cpp.ones(dims, shape, unit, dtype, variances)


def ones_like(var: _cpp.Variable) -> _cpp.Variable:
    """Constructs a :class:`Variable` with the same dims, shape, unit and dtype
    as the input variable, but with all values initialized to 1. If the input
    has variances, all variances in the output are set to 1.

    :seealso: :py:func:`scipp.ones` :py:func:`scipp.zeros_like`

    :param var: Input variable.
    """
    return ones(dims=var.dims,
                shape=var.shape,
                unit=var.unit,
                dtype=var.dtype,
                variances=var.variances is not None)


def empty(*,
          dims: _Sequence[str],
          shape: _Sequence[int],
          unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
          dtype: type(_cpp.dtype.float64) = _cpp.dtype.float64,
          variances: bool = False) -> _cpp.Variable:
    """Constructs a :class:`Variable` with uninitialized values with given
    dimension labels and shape. USE WITH CARE! Uninitialized means that values
    have undetermined values. Consider using :py:func:`scipp.zeros` unless you
    know what you are doing and require maximum performance.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones`
              :py:func:`scipp.scalar` :py:func:`scipp.array`

    :param dims: Dimension labels.
    :param shape: Dimension sizes.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param variances: Optional, boolean flag, if True includes uninitialized
                      variances. Default=False
    """
    return _cpp.empty(dims, shape, unit, dtype, variances)


def empty_like(var: _cpp.Variable) -> _cpp.Variable:
    """Constructs a :class:`Variable` with the same dims, shape, unit and dtype
    as the input variable, but with uninitialized values. If the input
    has variances, all variances in the output exist but are uninitialized.

    :seealso: :py:func:`scipp.empty` :py:func:`scipp.zeros_like`
              :py:func:`scipp.ones_like`

    :param var: Input variable.
    """
    return empty(dims=var.dims,
                 shape=var.shape,
                 unit=var.unit,
                 dtype=var.dtype,
                 variances=var.variances is not None)


def array(*,
          dims: _Sequence[str],
          values: _Union[_np.ndarray, list],
          variances: _Union[_np.ndarray, list] = None,
          unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
          dtype: type(_cpp.dtype.float64) = None) -> _cpp.Variable:
    """Constructs a :class:`Variable` with given dimensions, containing given
    values and optional variances. Dimension and value shape must match.
    Only keyword arguments accepted.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones`
              :py:func:`scipp.empty` :py:func:`scipp.scalar`

    :param dims: Dimension labels.
    :param values: Initial values.
    :param variances: Optional, initial variances, must be same shape
      and size as values. Default=None
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
    """
    return _cpp.Variable(dims=dims,
                         values=values,
                         variances=variances,
                         unit=unit,
                         dtype=dtype)


def linspace(dim: str,
             start: _Union[int, float],
             stop: _Union[int, float],
             num: int,
             *,
             unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
             dtype: type(_cpp.dtype.float64) = None) -> _cpp.Variable:
    """Constructs a :class:`Variable` with `num` evenly spaced samples,
    calculated over the interval `[start, stop]`.

    :seealso: :py:func:`scipp.geomspace` :py:func:`scipp.logspace`
              :py:func:`scipp.arange`

    :param dim: Dimension label.
    :param start: The starting value of the sequence.
    :param stop: The end value of the sequence.
    :param num: Number of samples to generate.
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
    """
    return array(dims=[dim],
                 values=_np.linspace(start, stop, num),
                 unit=unit,
                 dtype=dtype)


def geomspace(dim: str,
              start: _Union[int, float],
              stop: _Union[int, float],
              num: int,
              *,
              unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
              dtype: type(_cpp.dtype.float64) = None) -> _cpp.Variable:
    """Constructs a :class:`Variable` with values spaced evenly on a log scale
    (a geometric progression). This is similar to :py:func:`scipp.logspace`,
    but with endpoints specified directly.
    Each output sample is a constant multiple of the previous.

    :seealso: :py:func:`scipp.linspace` :py:func:`scipp.logspace`
              :py:func:`scipp.arange`

    :param dim: Dimension label.
    :param start: The starting value of the sequence.
    :param stop: The end value of the sequence.
    :param num: Number of samples to generate.
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
    """
    return array(dims=[dim],
                 values=_np.geomspace(start, stop, num),
                 unit=unit,
                 dtype=dtype)


def logspace(dim: str,
             start: _Union[int, float],
             stop: _Union[int, float],
             num: int,
             *,
             unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
             dtype: type(_cpp.dtype.float64) = None) -> _cpp.Variable:
    """Constructs a :class:`Variable` with values spaced evenly on a log scale.

    :seealso: :py:func:`scipp.linspace` :py:func:`scipp.geomspace`
              :py:func:`scipp.arange`

    :param dim: Dimension label.
    :param start: The starting value of the sequence.
    :param stop: The end value of the sequence.
    :param num: Number of samples to generate.
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
    """
    return array(dims=[dim],
                 values=_np.logspace(start, stop, num),
                 unit=unit,
                 dtype=dtype)


def arange(dim: str,
           start: _Union[int, float],
           stop: _Union[int, float] = None,
           step: _Union[int, float] = 1,
           *,
           unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
           dtype: type(_cpp.dtype.float64) = None) -> _cpp.Variable:
    """Constructs a :class:`Variable` with evenly spaced values within a given
    interval.
    Values are generated within the half-open interval [start, stop)
    (in other words, the interval including start but excluding stop).

    :seealso: :py:func:`scipp.linspace` :py:func:`scipp.geomspace`
              :py:func:`scipp.logspace`

    :param dim: Dimension label.
    :param start: Optional, the starting value of the sequence. Default=0.
    :param stop: End of interval. The interval does not include this value,
      except in some (rare) cases where step is not an integer and floating
      point round-off can come into play.
    :param step: Optional, spacing between values. Default=1.
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
    """
    if stop is None:
        stop = start
        start = 0
    return array(dims=[dim],
                 values=_np.arange(start, stop, step),
                 unit=unit,
                 dtype=dtype)


# Wrapper to make datetime usable without importing numpy manually.
datetime64 = _np.datetime64
