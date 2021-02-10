# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from . import units, Unit, dtype, Variable
from typing import Any, Sequence, Union
import numpy


def scalar(value: Any,
           variance: Any = None,
           unit: Unit = units.dimensionless,
           dtype: type(dtype.float64) = None) -> Variable:
    """Constructs a zero dimensional :class:`Variable` with a unit and optional
    variance.

    :param value: Initial value.
    :param variance: Optional, initial variance, Default=None
    :param unit: Optional, unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
      Cannot be specified for value types of
      str, Dataset or DataArray.
    :raises: Add this
    """
    if dtype is None:
        return Variable(value=value, variance=variance, unit=unit)
    else:
        try:
            return Variable(value=value,
                            variance=variance,
                            unit=unit,
                            dtype=dtype)
        except TypeError:
            # Raise a more comprehensible error message in the case
            # where a dtype cannot be specified.
            raise TypeError(f"Cannot convert {value} to {dtype}. "
                            f"Try omitting the 'dtype=' parameter.")


def zeros(*,
          dims: Sequence[str],
          shape: Sequence[int],
          unit: Unit = units.dimensionless,
          dtype: type(dtype.float64) = dtype.float64,
          variances: bool = False) -> Variable:
    """Constructs a :class:`Variable` with default initialised values with
    given dimension labels and shape.
    Optionally can add default initialised variances.
    Only keyword arguments accepted.

    :param dims: Dimension labels.
    :param shape: Dimension sizes.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param variances: Optional, boolean flag, if True includes variances
      initialised to the default value for dtype.
      For example for a float type values and variances would all be
      initialised to 0.0. Default=False
    """
    return Variable(dims=dims,
                    shape=shape,
                    unit=unit,
                    dtype=dtype,
                    variances=variances)


def array(*,
          dims: Sequence[str],
          values: Union[numpy.ndarray, list],
          variances: Union[numpy.ndarray, list] = None,
          unit: Unit = units.dimensionless,
          dtype: type(dtype.float64) = None) -> Variable:
    """Constructs a :class:`Variable` with given dimensions, containing given
    values and optional variances. Dimension and value shape must match.
    Only keyword arguments accepted.

    :param dims: Dimension labels.
    :param values: Initial values.
    :param variances: Optional, initial variances, must be same shape
      and size as values. Default=None
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
    """
    return Variable(dims=dims,
                    values=values,
                    variances=variances,
                    unit=unit,
                    dtype=dtype)


# Wrapper to make dateime usable without importing numpy manually.
datetime64 = numpy.datetime64
