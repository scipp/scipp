# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from typing import Union as _Union

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def isnan(x: _cpp.Variable) -> _cpp.Variable:
    """Element-wise isnan (true if an element is nan). """
    return _call_cpp_func(_cpp.isnan, x)


def isinf(x: _cpp.Variable) -> _cpp.Variable:
    """Element-wise isinf (true if an element is inf)."""
    return _call_cpp_func(_cpp.isinf, x)


def isfinite(x: _cpp.Variable) -> _cpp.Variable:
    """Element-wise isfinite (true if an element is finite)."""
    return _call_cpp_func(_cpp.isfinite, x)


def isposinf(x: _cpp.Variable) -> _cpp.Variable:
    """Element-wise isposinf (true if an element is a positive infinity)."""
    return _call_cpp_func(_cpp.isposinf, x)


def isneginf(x: _cpp.Variable) -> _cpp.Variable:
    """Element-wise isneginf (true if an element is a negative infinity)."""
    return _call_cpp_func(_cpp.isneginf, x)


def to_unit(x: _cpp.Variable,
            unit: _Union[_cpp.Unit, str],
            *,
            copy: bool = True) -> _cpp.Variable:
    """Convert the variable to a different unit.

    Raises a :class:`scipp.UnitError` if the input unit is not compatible
    with the provided unit, e.g., `m` cannot be converted to `s`.

    If the input dtype is an integer type or datetime64, the output is rounded
    and returned with the same dtype as the input.

    Parameters
    ----------
    x:
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
      <scipp.Variable> ()    float64             [mm]  [1200]
    """
    return _call_cpp_func(_cpp.to_unit, x=x, unit=unit, copy=copy)
