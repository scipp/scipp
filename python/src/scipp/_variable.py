# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from . import units, Unit, dtype
from typing import Any, Sequence


def scalar(value: Any, variance: Any = None, unit: Unit = units.one):
    """Constructs a zero dimensional :class:`Variable` with a unit and optional
    variance.

    :param value: Initial value.
    :param variance: Optional, initial variance, Default=None
    :param unit: Optional, unit. Default=dimensionless
    :type value: Any
    :type variance: Same type as value
    :type unit: Unit
    """
    return _cpp.Variable(value=value, variance=variance, unit=unit)


def zeros(*, dims: Sequence[str], shape: Sequence[int],
          unit: Unit = units.one, dtype: dtype = dtype.float64,
          variances: bool = False):
    """Constructs a default initialised :class:`Variable` with given dimension
    labels and shape. Only keyword arguments accepted.

    :param dims: Dimension labels.
    :param shape: Dimension sizes.
    :param unit: Optional, unit of contents. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=float64
    :param variance: Optional, boolean flag, if True include variances.
      Default=False
    :type dims: list[str]
    :type shape: list[int]
    :type unit: Unit
    :type variance: bool
    :type dtype: dtype
    """
    return _cpp.Variable(dims=dims, shape=shape, unit=unit,
                         dtype=dtype, variances=variances)


def array(*, dims: Sequence[str], values, variances=None,
          unit: Unit = units.one, dtype: dtype = None):
    """Constructs a :class:`Variable` with given dimensions, containing given
    values. Dimension and value shape must match.
    Only keyword arguments accepted.

    :param dims: Dimension labels.
    :param values: Initial values.
    :param variances: Optional, initial variances, must be same shape
      and size as values. Default=None
    :param unit: Optional, data unit. Default=dimensionless
    :param dtype: Optional, type of underlying data. Default=None,
      in which case type is inferred from value input.
    :type dims: list[str]
    :type values: numpy.ndarray, list
    :type variances: numpy.ndarray, list
    :type unit: Unit
    :type dtype: dtype
    """
    return _cpp.Variable(dims=dims, values=values, variances=variances,
                         unit=unit, dtype=dtype)
