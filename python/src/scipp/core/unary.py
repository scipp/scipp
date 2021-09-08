# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from typing import Union as _Union

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def isnan(x: _cpp.Variable) -> _cpp.Variable:
    """
    Element-wise isnan (true if an element is nan).
    """
    return _call_cpp_func(_cpp.isnan, x)


def isinf(x: _cpp.Variable) -> _cpp.Variable:
    """
    Element-wise isinf (true if an element is inf).
    """
    return _call_cpp_func(_cpp.isinf, x)


def isfinite(x: _cpp.Variable) -> _cpp.Variable:
    """
    Element-wise isfinite (true if an element is finite).
    """
    return _call_cpp_func(_cpp.isfinite, x)


def isposinf(x: _cpp.Variable) -> _cpp.Variable:
    """
    Element-wise isposinf (true if an element is a positive infinity).
    """
    return _call_cpp_func(_cpp.isposinf, x)


def isneginf(x: _cpp.Variable) -> _cpp.Variable:
    """
    Element-wise isneginf (true if an element is a negative infinity).
    """
    return _call_cpp_func(_cpp.isneginf, x)


def to_unit(x: _cpp.Variable,
            unit: _Union[_cpp.Unit, str],
            *,
            copy: bool = True) -> _cpp.Variable:
    """
    Convert the variable to a different unit.

    Example:

    .. code-block:: python

        var = 1.2 * sc.Unit('m')
        var_in_mm = sc.to_unit(var, unit='mm')

    If the unit us unchanged and ``copy`` is `False`, the object
    is returned without making a deep copy.

    Raises an error if the input unit is not compatible with the provided
    unit, e.g., `m` cannot be converted to `s`.

    If the input dtype is an integer type or datetime64, the output is rounded
    and returned with the same dtype as the input.

    :param x: Input variable.
    :param unit: Desired target unit.
    :param copy: If `False`, return the input object if possible.
                 If `True`, the function always returns a new object.
    """
    return _call_cpp_func(_cpp.to_unit, x=x, unit=unit, copy=copy)
