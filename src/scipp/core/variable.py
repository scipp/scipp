# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
# flake8: noqa: E501

from __future__ import annotations

import warnings
from collections.abc import Iterable as _Iterable
from typing import Any, Optional, Sequence, TypeVar, Union

import numpy as _np
from numpy.typing import ArrayLike as array_like

from .._scipp import core as _cpp
from .cpp_classes import DType, Unit, Variable
from ..units import default_unit
from ._sizes import _parse_dims_shape_sizes
from ..typing import DTypeLike

NumberOrVar = TypeVar('NumberOrVar', Union[int, float], Variable)


def scalar(value: Any,
           variance: Any = None,
           unit: Union[Unit, str, None] = default_unit,
           dtype: Optional[DTypeLike] = None) -> Variable:
    """Constructs a zero dimensional :class:`Variable` with a unit and optional
    variance.


    :param value: Initial value.
    :param variance: Optional, initial variance, Default=None
    :param unit: Optional, unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
      Cannot be specified for value types of
      str, Dataset or DataArray.
    :returns: A scalar (zero-dimensional) Variable.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones`
              :py:func:`scipp.empty` :py:func:`scipp.array`
    """
    return _cpp.Variable(dims=(),
                         values=value,
                         variances=variance,
                         unit=unit,
                         dtype=dtype)


def index(value: Any, dtype: Optional[DTypeLike] = None) -> Variable:
    """Constructs a zero dimensional :class:`Variable` representing an index.

    This is equivalent to calling :py:func:`scipp.scalar` with unit=None.

    :param value: Initial value.
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
    :returns: A scalar (zero-dimensional) variable without unit.
    """
    return scalar(value=value, dtype=dtype, unit=None)


def zeros(*,
          dims: Sequence[str] = None,
          shape: Sequence[int] = None,
          sizes: dict = None,
          unit: Union[Unit, str, None] = default_unit,
          dtype: DTypeLike = DType.float64,
          with_variances: bool = False) -> Variable:
    """Constructs a :class:`Variable` with default initialized values with
    given dimension labels and shape.
    The dims and shape can also be specified using a sizes dict.
    Optionally can add default initialized variances.
    Only keyword arguments accepted.


    :param dims: Optional (if sizes is specified), dimension labels.
    :param shape: Optional (if sizes is specified), dimension sizes.
    :param sizes: Optional, dimension label to size map.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param with_variances: Optional, boolean flag, if True includes variances
      initialized to the default value for dtype.
      For example for a float type values and variances would all be
      initialized to 0.0. Default=False

    :seealso: :py:func:`scipp.ones` :py:func:`scipp.empty`
              :py:func:`scipp.scalar` :py:func:`scipp.array`
    """

    return _cpp.zeros(**_parse_dims_shape_sizes(dims, shape, sizes),
                      unit=unit,
                      dtype=dtype,
                      with_variances=with_variances)


def ones(*,
         dims: Sequence[str] = None,
         shape: Sequence[int] = None,
         sizes: dict = None,
         unit: Union[Unit, str, None] = default_unit,
         dtype: DTypeLike = DType.float64,
         with_variances: bool = False) -> Variable:
    """Constructs a :class:`Variable` with values initialized to 1 with
    given dimension labels and shape.
    The dims and shape can also be specified using a sizes dict.


    :param dims: Optional (if sizes is specified), dimension labels.
    :param shape: Optional (if sizes is specified), dimension sizes.
    :param sizes: Optional, dimension label to size map.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param with_variances: Optional, boolean flag, if True includes variances
                      initialized to 1. Default=False

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.empty`
              :py:func:`scipp.scalar` :py:func:`scipp.array`
    """
    return _cpp.ones(**_parse_dims_shape_sizes(dims, shape, sizes),
                     unit=unit,
                     dtype=dtype,
                     with_variances=with_variances)


def empty(*,
          dims: Sequence[str] = None,
          shape: Sequence[int] = None,
          sizes: dict = None,
          unit: Union[Unit, str, None] = default_unit,
          dtype: DTypeLike = DType.float64,
          with_variances: bool = False) -> Variable:
    """Constructs a :class:`Variable` with uninitialized values with given
    dimension labels and shape.
    The dims and shape can also be specified using a sizes dict.
    USE WITH CARE! Uninitialized means that values have undetermined values.
    Consider using :py:func:`scipp.zeros` unless you
    know what you are doing and require maximum performance.


    :param dims: Optional (if sizes is specified), dimension labels.
    :param shape: Optional (if sizes is specified), dimension sizes.
    :param sizes: Optional, dimension label to size map.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param with_variances: Optional, boolean flag, if True includes
                    uninitialized variances. Default=False

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones`
              :py:func:`scipp.scalar` :py:func:`scipp.array`
    """
    return _cpp.empty(**_parse_dims_shape_sizes(dims, shape, sizes),
                      unit=unit,
                      dtype=dtype,
                      with_variances=with_variances)


def full(*,
         dims: Sequence[str] = None,
         shape: Sequence[int] = None,
         sizes: dict = None,
         unit: Union[Unit, str, None] = default_unit,
         dtype: Optional[DTypeLike] = None,
         value: Any,
         variance: Any = None) -> Variable:
    """
    Constructs a :class:`Variable` with values initialized to the specified
    value with given dimension labels and shape.
    The dims and shape can also be specified using a sizes dict.


    :param dims: Optional (if sizes is specified), dimension labels.
    :param shape: Optional (if sizes is specified), dimension sizes.
    :param sizes: Optional, dimension label to size map.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Deduced from 'value' if not given.
    :param value: The value to fill the Variable with
    :param variance: Optional, the variance to fill the Variable with. If None
        or not provided, the variances will not be set.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones`
    """
    return scalar(value=value, variance=variance, unit=unit, dtype=dtype)\
        .broadcast(**_parse_dims_shape_sizes(dims, shape, sizes)).copy()


def matrix(*,
           unit: Union[Unit, str, None] = default_unit,
           value: Union[ndarray, list]) -> Variable:
    """Constructs a zero dimensional :class:`Variable` holding a single 3x3
    matrix.


    :param value: Initial value, a list or 1-D numpy array.
    :param unit: Optional, unit. Default=dimensionless
    :returns: A scalar (zero-dimensional) Variable.

    :seealso: :py:func:`scipp.matrices`
    """
    warnings.warn(
        "sc.matrix() has been deprecated in favour of "
        "sc.spatial.linear_transform(), and will be removed in a future "
        "version of scipp.", DeprecationWarning)
    from ..spatial import linear_transform
    return linear_transform(unit=unit, value=value)


def matrices(*,
             dims: Sequence[str],
             unit: Union[Unit, str, None] = default_unit,
             values: Union[_np.ndarray, list]) -> Variable:
    """Constructs a :class:`Variable` with given dimensions holding an array
    of 3x3 matrices.


    :param dims: Dimension labels.
    :param values: Initial values.
    :param unit: Optional, data unit. Default=dimensionless

    :seealso: :py:func:`scipp.matrix`
    """
    warnings.warn(
        "sc.matrices() has been deprecated in favour of "
        "sc.spatial.linear_transforms(), and will be removed in a future "
        "version of scipp.", DeprecationWarning)
    from ..spatial import linear_transforms
    return linear_transforms(dims=dims, unit=unit, values=values)


def vector(*,
           unit: Union[Unit, str, None] = default_unit,
           value: Union[_np.ndarray, list]) -> Variable:
    """Constructs a zero dimensional :class:`Variable` holding a single length-3
    vector.


    :param value: Initial value, a list or 1-D numpy array.
    :param unit: Optional, unit. Default=dimensionless
    :returns: A scalar (zero-dimensional) Variable.

    :seealso: :py:func:`scipp.vectors`
    """
    return _cpp.vectors(dims=[], unit=unit, values=value)


def vectors(*,
            dims: Sequence[str],
            unit: Union[Unit, str, None] = default_unit,
            values: Union[_np.ndarray, list]) -> Variable:
    """Constructs a :class:`Variable` with given dimensions holding an array
    of length-3 vectors.


    :param dims: Dimension labels.
    :param values: Initial values.
    :param unit: Optional, data unit. Default=dimensionless

    :seealso: :py:func:`scipp.vector`
    """
    return _cpp.vectors(dims=dims, unit=unit, values=values)


def array(*,
          dims: _Iterable[str],
          values: array_like,
          variances: Optional[array_like] = None,
          unit: Union[Unit, str, None] = default_unit,
          dtype: Optional[DTypeLike] = None) -> Variable:
    """Constructs a :class:`Variable` with given dimensions, containing given
    values and optional variances. Dimension and value shape must match.
    Only keyword arguments accepted.


    :param dims: Dimension labels
    :param values: Initial values.
    :param variances: Optional, initial variances, must be same shape
      and size as values. Default=None
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones`
              :py:func:`scipp.empty` :py:func:`scipp.scalar`
    """
    return _cpp.Variable(dims=dims,
                         values=values,
                         variances=variances,
                         unit=unit,
                         dtype=dtype)


# Assumes that all arguments are Variable or None.
def _ensure_same_unit(*, unit, args: dict):
    if unit == default_unit:
        units = {key: val.unit for key, val in args.items() if val is not None}
        if len(set(units.values())) != 1:
            raise _cpp.UnitError(
                f'All units of the following arguments must be equal: {units}. '
                'You can specify a unit explicitly with the `unit` argument.')
        unit = next(iter(units.values()))
    return {
        key: _cpp.to_unit(val, unit, copy=False).value if val is not None else None
        for key, val in args.items()
    }, unit


# Process arguments of arange, linspace, etc and return them as plain numbers or None.
def _normalize_range_args(*, unit, **kwargs):
    is_var = {
        key: isinstance(val, _cpp.Variable)
        for key, val in kwargs.items() if val is not None
    }
    if any(is_var.values()):
        if not all(is_var.values()):
            arg_types = {key: type(val) for key, val in kwargs.items()}
            raise TypeError('Either all of the following arguments or none have to '
                            f'be variables: {arg_types}')
        return _ensure_same_unit(unit=unit, args=kwargs)
    return kwargs, unit


def linspace(dim: str,
             start: NumberOrVar,
             stop: NumberOrVar,
             num: int,
             *,
             endpoint: bool = True,
             unit: Union[Unit, str, None] = default_unit,
             dtype: Optional[DTypeLike] = None) -> Variable:
    """Constructs a :class:`Variable` with `num` evenly spaced samples,
    calculated over the interval `[start, stop]`.

    :param dim: Dimension label.
    :param start: The starting value of the sequence.
    :param stop: The end value of the sequence.
    :param num: Number of samples to generate.
    :param endpoint: If True, `step` is the last returned value.
                     Otherwise, it is not included.
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.

    :seealso: :py:func:`scipp.geomspace` :py:func:`scipp.logspace`
              :py:func:`scipp.arange`
    """
    range_args, unit = _normalize_range_args(unit=unit, start=start, stop=stop)
    return array(dims=[dim],
                 values=_np.linspace(**range_args, num=num, endpoint=endpoint),
                 unit=unit,
                 dtype=dtype)


def geomspace(dim: str,
              start: Union[int, float],
              stop: Union[int, float],
              num: int,
              *,
              endpoint: bool = True,
              unit: Union[Unit, str, None] = default_unit,
              dtype: Optional[DTypeLike] = None) -> Variable:
    """Constructs a :class:`Variable` with values spaced evenly on a log scale
    (a geometric progression).

    This is similar to :py:func:`scipp.logspace`, but with endpoints specified
    directly instead of as exponents.
    Each output sample is a constant multiple of the previous.

    :param dim: Dimension label.
    :param start: The starting value of the sequence.
    :param stop: The end value of the sequence.
    :param num: Number of samples to generate.
    :param endpoint: If True, `step` is the last returned value.
                     Otherwise, it is not included.
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.

    :seealso: :py:func:`scipp.linspace` :py:func:`scipp.logspace`
              :py:func:`scipp.arange` :py:func:`numpy.geomspace`
    """
    return array(dims=[dim],
                 values=_np.geomspace(start, stop, num, endpoint=endpoint),
                 unit=unit,
                 dtype=dtype)


def logspace(dim: str,
             start: Union[int, float],
             stop: Union[int, float],
             num: int,
             *,
             endpoint: bool = True,
             base: Union[int, float] = 10.0,
             unit: Union[Unit, str, None] = default_unit,
             dtype: Optional[DTypeLike] = None) -> Variable:
    """Constructs a :class:`Variable` with values spaced evenly on a log scale.

    This is similar to :py:func:`scipp.geomspace`, but with endpoints specified
    as exponents.

    :param dim: Dimension label.
    :param start: ``base ** start`` is the starting value of the sequence.
    :param stop: ``base ** end`` is the end value of the sequence.
    :param num: Number of samples to generate.
    :param base: The base of the log space.
    :param endpoint: If True, `step` is the last returned value.
                     Otherwise, it is not included.
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.

    :seealso: :py:func:`scipp.linspace` :py:func:`scipp.geomspace`
              :py:func:`scipp.arange` :py:func:`numpy.logspace`
    """
    return array(dims=[dim],
                 values=_np.logspace(start, stop, num, base=base, endpoint=endpoint),
                 unit=unit,
                 dtype=dtype)


def arange(dim: str,
           start: Union[NumberOrVar, _np.datetime64],
           stop: Optional[Union[NumberOrVar, _np.datetime64]] = None,
           step: Optional[NumberOrVar] = None,
           *,
           unit: Union[Unit, str, None] = default_unit,
           dtype: Optional[DTypeLike] = None) -> Variable:
    """Constructs a :class:`Variable` with evenly spaced values within a given
    interval.
    Values are generated within the half-open interval [start, stop)
    (in other words, the interval including start but excluding stop).

    :param dim: Dimension label.
    :param start: Optional, the starting value of the sequence. Default=0.
    :param stop: End of interval. The interval does not include this value,
      except in some (rare) cases where step is not an integer and floating-
      point round-off can come into play.
    :param step: Optional, spacing between values. Default=1.
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.

    :seealso: :py:func:`scipp.linspace` :py:func:`scipp.geomspace`
              :py:func:`scipp.logspace`
    """
    range_args, unit = _normalize_range_args(unit=unit,
                                             start=start,
                                             stop=stop,
                                             step=step)
    return array(dims=[dim], values=_np.arange(**range_args), unit=unit, dtype=dtype)


def datetime(value: Union[str, int, _np.datetime64],
             *,
             unit: Optional[Union[Unit, str, None]] = default_unit) -> Variable:
    """Constructs a zero dimensional :class:`Variable` with a dtype of datetime64.

    :param value:
     - `str`: Interpret the string according to the ISO 8601 date time format.
     - `int`: Number of time units (see argument ``unit``) since scipp's epoch
              (see :py:func:`scipp.epoch`).
     - `np.datetime64`: Construct equivalent datetime of scipp.
    :param unit: Unit of the resulting datetime.
                 Can be deduced if ``value`` is a str or np.datetime64 but
                 is required if it is an int.

    :seealso: :py:func:`scipp.datetimes` :py:func:`scipp.epoch`
              'Dates and Times' section in `Data Types <../../reference/dtype.rst>`_

    Examples:

      >>> sc.datetime('2021-01-10T14:16:15')
      <scipp.Variable> ()  datetime64              [s]  [2021-01-10T14:16:15]
      >>> sc.datetime('2021-01-10T14:16:15', unit='ns')
      <scipp.Variable> ()  datetime64             [ns]  [2021-01-10T14:16:15.000000000]
      >>> sc.datetime(1610288175, unit='s')
      <scipp.Variable> ()  datetime64              [s]  [2021-01-10T14:16:15]
    """
    if isinstance(value, str):
        return scalar(_np.datetime64(value), unit=unit)
    return scalar(value, unit=unit, dtype=_cpp.DType.datetime64)


def datetimes(*,
              dims,
              values: array_like,
              unit: Optional[Union[Unit, str, None]] = default_unit) -> Variable:
    """Constructs an array :class:`Variable` with a dtype of datetime64.

    :param dims: Dimension labels
    :param values: Numpy array or something that can be converted to a
                   Numpy array of datetimes.
    :param unit: Unit for the resulting Variable.
                 Can be deduced if ``values`` contains strings or np.datetime64's.

    :seealso: :py:func:`scipp.datetime` :py:func:`scipp.epoch`
              'Dates and Times' section in `Data Types <../../reference/dtype.rst>`_

    Examples:

      >>> sc.datetimes(dims=['t'], values=['2021-01-10T01:23:45', '2021-01-11T01:23:45'])
      <scipp.Variable> (t: 2)  datetime64              [s]  [2021-01-10T01:23:45, 2021-01-11T01:23:45]
      >>> sc.datetimes(dims=['t'], values=['2021-01-10T01:23:45', '2021-01-11T01:23:45'], unit='h')
      <scipp.Variable> (t: 2)  datetime64              [h]  [2021-01-10T01:00:00, 2021-01-11T01:00:00]
      >>> sc.datetimes(dims=['t'], values=[0, 1610288175], unit='s')
      <scipp.Variable> (t: 2)  datetime64              [s]  [1970-01-01T00:00:00, 2021-01-10T14:16:15]
    """
    if unit is None or unit is default_unit:
        np_unit_str = ''
    else:
        np_unit_str = f'[{_cpp.to_numpy_time_string(unit)}]'
    return array(dims=dims,
                 values=_np.asarray(values, dtype=f'datetime64{np_unit_str}'))


def epoch(*, unit: Union[Unit, str]) -> Variable:
    """Constructs a zero dimensional :class:`Variable` with a dtype of
    datetime64 that contains scipp's epoch.

    Currently, the epoch of datetimes in scipp is the Unix epoch 1970-01-01T00:00:00.

    :param unit: Unit of the resulting Variable.

    :seealso: :py:func:`scipp.datetime` :py:func:`scipp.datetimes`
              'Dates and Times' section in `Data Types <../../reference/dtype.rst>`_

    Examples:

      >>> sc.epoch(unit='s')
      <scipp.Variable> ()  datetime64              [s]  [1970-01-01T00:00:00]
    """
    return scalar(0, unit=unit, dtype=_cpp.DType.datetime64)
