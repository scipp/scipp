# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew

from __future__ import annotations
from collections.abc import Iterable as _Iterable
from typing import Any as _Any, Sequence as _Sequence, Union as _Union,\
    Optional as _Optional

import numpy as _np
from numpy.typing import ArrayLike as array_like

from .._scipp import core as _cpp


def _parse_dims_shape_sizes(dims, shape, sizes):
    if sizes is not None:
        if dims is not None or shape is not None:
            raise ValueError("When sizes is specified, dims and shape must "
                             "both be None. Got dims: {}, shape: {}".format(
                                 dims, shape))
        dims = list(sizes.keys())
        shape = list(sizes.values())
    return {"dims": dims, "shape": shape}


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
    return _cpp.Variable(dims=(),
                         values=value,
                         variances=variance,
                         unit=unit,
                         dtype=dtype)


def zeros(*,
          dims: _Sequence[str] = None,
          shape: _Sequence[int] = None,
          sizes: dict = None,
          unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
          dtype: type(_cpp.dtype.float64) = _cpp.dtype.float64,
          with_variances: bool = False) -> _cpp.Variable:
    """Constructs a :class:`Variable` with default initialized values with
    given dimension labels and shape.
    The dims and shape can also be specified using a sizes dict.
    Optionally can add default initialized variances.
    Only keyword arguments accepted.

    :seealso: :py:func:`scipp.ones` :py:func:`scipp.empty`
              :py:func:`scipp.scalar` :py:func:`scipp.array`

    :param dims: Optional (if sizes is specified), dimension labels.
    :param shape: Optional (if sizes is specified), dimension sizes.
    :param sizes: Optional, imension label to size map.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param with_variances: Optional, boolean flag, if True includes variances
      initialized to the default value for dtype.
      For example for a float type values and variances would all be
      initialized to 0.0. Default=False
    """

    return _cpp.zeros(**_parse_dims_shape_sizes(dims, shape, sizes),
                      unit=unit,
                      dtype=dtype,
                      with_variances=with_variances)


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
                 with_variances=var.variances is not None)


def ones(*,
         dims: _Sequence[str] = None,
         shape: _Sequence[int] = None,
         sizes: dict = None,
         unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
         dtype: type(_cpp.dtype.float64) = _cpp.dtype.float64,
         with_variances: bool = False) -> _cpp.Variable:
    """Constructs a :class:`Variable` with values initialized to 1 with
    given dimension labels and shape.
    The dims and shape can also be specified using a sizes dict.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.empty`
              :py:func:`scipp.scalar` :py:func:`scipp.array`

    :param dims: Optional (if sizes is specified), dimension labels.
    :param shape: Optional (if sizes is specified), dimension sizes.
    :param sizes: Optional, dimension label to size map.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param with_variances: Optional, boolean flag, if True includes variances
                      initialized to 1. Default=False
    """
    return _cpp.ones(**_parse_dims_shape_sizes(dims, shape, sizes),
                     unit=unit,
                     dtype=dtype,
                     with_variances=with_variances)


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
                with_variances=var.variances is not None)


def empty(*,
          dims: _Sequence[str] = None,
          shape: _Sequence[int] = None,
          sizes: dict = None,
          unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
          dtype: type(_cpp.dtype.float64) = _cpp.dtype.float64,
          with_variances: bool = False) -> _cpp.Variable:
    """Constructs a :class:`Variable` with uninitialized values with given
    dimension labels and shape.
    The dims and shape can also be specified using a sizes dict.
    USE WITH CARE! Uninitialized means that values have undetermined values.
    Consider using :py:func:`scipp.zeros` unless you
    know what you are doing and require maximum performance.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones`
              :py:func:`scipp.scalar` :py:func:`scipp.array`

    :param dims: Optional (if sizes is specified), dimension labels.
    :param shape: Optional (if sizes is specified), dimension sizes.
    :param sizes: Optional, dimension label to size map.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param with_variances: Optional, boolean flag, if True includes
                    uninitialized variances. Default=False
    """
    return _cpp.empty(**_parse_dims_shape_sizes(dims, shape, sizes),
                      unit=unit,
                      dtype=dtype,
                      with_variances=with_variances)


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
                 with_variances=var.variances is not None)


def full(*,
         dims: _Sequence[str] = None,
         shape: _Sequence[int] = None,
         sizes: dict = None,
         unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
         dtype: type(_cpp.dtype.float64) = _cpp.dtype.float64,
         value: _Any,
         variance: _Any = None) -> _cpp.Variable:
    """
    Constructs a :class:`Variable` with values initialized to the specified
    value with given dimension labels and shape.
    The dims and shape can also be specified using a sizes dict.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones`

    :param dims: Optional (if sizes is specified), dimension labels.
    :param shape: Optional (if sizes is specified), dimension sizes.
    :param sizes: Optional, dimension label to size map.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param value: The value to fill the Variable with
    :param variance: Optional, the variance to fill the Variable with. If None
        or not provided, the variances will not be set.
    """
    return scalar(value=value, variance=variance, unit=unit, dtype=dtype)\
        .broadcast(**_parse_dims_shape_sizes(dims, shape, sizes)).copy()


def full_like(var: _cpp.Variable, value: _Any, variance: _Any = None) -> _cpp.Variable:
    """
    Constructs a :class:`Variable` with values initialized to the specified
    value with dimensions labels and shape provided by an existing variable.

    :seealso: :py:func:`scipp.zeros_like` :py:func:`scipp.ones_like`

    :param var: Input variable to copy dimensions, sizes, unit and dtype from.
    :param value: The value to fill the Variable with
    :param variance: Optional, the variance to fill the Variable with. If None
        or not provided, the variances will not be set.
    """
    return full(dims=var.dims,
                shape=var.shape,
                unit=var.unit,
                dtype=var.dtype,
                value=value,
                variance=variance)


def _to_eigen_layout(a):
    # Numpy and scipp use row-major, but Eigen matrices use column-major,
    # transpose matrix axes for copying values.
    return _np.moveaxis(a, -1, -2)


def matrix(*,
           unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
           value: _Union[_np.ndarray, list]):
    """Constructs a zero dimensional :class:`Variable` holding a single 3x3
    matrix.

    :seealso: :py:func:`scipp.matrices`

    :param value: Initial value, a list or 1-D numpy array.
    :param unit: Optional, unit. Default=dimensionless
    :returns: A scalar (zero-dimensional) Variable.
    :rtype: Variable
    """
    return _cpp.matrices(dims=[], unit=unit, values=_to_eigen_layout(value))


def matrices(*,
             dims: _Sequence[str],
             unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
             values: _Union[_np.ndarray, list]):
    """Constructs a :class:`Variable` with given dimensions holding an array
    of 3x3 matrices.

    :seealso: :py:func:`scipp.matrix`

    :param dims: Dimension labels.
    :param values: Initial values.
    :param unit: Optional, data unit. Default=dimensionless
    """
    return _cpp.matrices(dims=dims, unit=unit, values=_to_eigen_layout(values))


def vector(*,
           unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
           value: _Union[_np.ndarray, list]):
    """Constructs a zero dimensional :class:`Variable` holding a single length-3
    vector.

    :seealso: :py:func:`scipp.vectors`

    :param value: Initial value, a list or 1-D numpy array.
    :param unit: Optional, unit. Default=dimensionless
    :returns: A scalar (zero-dimensional) Variable.
    :rtype: Variable
    """
    return _cpp.vectors(dims=[], unit=unit, values=value)


def vectors(*,
            dims: _Sequence[str],
            unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
            values: _Union[_np.ndarray, list]):
    """Constructs a :class:`Variable` with given dimensions holding an array
    of length-3 vectors.

    :seealso: :py:func:`scipp.vector`

    :param dims: Dimension labels.
    :param values: Initial values.
    :param unit: Optional, data unit. Default=dimensionless
    """
    return _cpp.vectors(dims=dims, unit=unit, values=values)


def array(*,
          dims: _Iterable,
          values: array_like,
          variances: _Optional[array_like] = None,
          unit: _Union[_cpp.Unit, str] = _cpp.units.dimensionless,
          dtype: type(_cpp.dtype.float64) = None) -> _cpp.Variable:
    """Constructs a :class:`Variable` with given dimensions, containing given
    values and optional variances. Dimension and value shape must match.
    Only keyword arguments accepted.

    :seealso: :py:func:`scipp.zeros` :py:func:`scipp.ones`
              :py:func:`scipp.empty` :py:func:`scipp.scalar`

    :param dims: Dimension labels
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
           start: _Union[int, float, _np.datetime64],
           stop: _Union[int, float, _np.datetime64] = None,
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
      except in some (rare) cases where step is not an integer and floating-
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
