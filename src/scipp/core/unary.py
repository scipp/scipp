# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew

from .._scipp import core as _cpp
from ..typing import VariableLikeType
from ._cpp_wrapper_util import call_func as _call_cpp_func


def isnan(x: VariableLikeType) -> VariableLikeType:
    """Element-wise isnan (true if an element is nan)."""
    return _call_cpp_func(_cpp.isnan, x)  # type: ignore[return-value]


def isinf(x: VariableLikeType) -> VariableLikeType:
    """Element-wise isinf (true if an element is inf)."""
    return _call_cpp_func(_cpp.isinf, x)  # type: ignore[return-value]


def isfinite(x: VariableLikeType) -> VariableLikeType:
    """Element-wise isfinite (true if an element is finite)."""
    return _call_cpp_func(_cpp.isfinite, x)  # type: ignore[return-value]


def isposinf(x: VariableLikeType) -> VariableLikeType:
    """Element-wise isposinf (true if an element is a positive infinity)."""
    return _call_cpp_func(_cpp.isposinf, x)  # type: ignore[return-value]


def isneginf(x: VariableLikeType) -> VariableLikeType:
    """Element-wise isneginf (true if an element is a negative infinity)."""
    return _call_cpp_func(_cpp.isneginf, x)  # type: ignore[return-value]


def to_unit(
    x: VariableLikeType, unit: _cpp.Unit | str, *, copy: bool = True
) -> VariableLikeType:
    """Convert the variable to a different unit.

    Raises a :class:`scipp.UnitError` if the input unit is not compatible
    with the provided unit, e.g., `m` cannot be converted to `s`.

    If the input dtype is an integer type or datetime64, the output is rounded
    and returned with the same dtype as the input.

    Parameters
    ----------
    x: VariableLike
        Input variable.
    unit:
        Desired target unit.
    copy:
        If `False`, return the input object if possible, i.e.
        if the unit is unchanged.
        If `True`, the function always returns a new object.

    Returns
    -------
    :
        Input converted to the given unit.

    Examples
    --------

      >>> var = 1.2 * sc.Unit('m')
      >>> sc.to_unit(var, unit='mm')
      <scipp.Variable> ()    float64             [mm]  1200
    """
    return _call_cpp_func(_cpp.to_unit, x=x, unit=unit, copy=copy)  # type: ignore[return-value]
