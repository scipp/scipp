# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def isnan(x: _cpp.Variable) -> bool:
    """
    Returns true if any element of the variable is nan.
    """
    _call_cpp_func(_cpp.isnan, x)


def isinf(x: _cpp.Variable) -> bool:
    """
    Returns true if any element of the variable is inf.
    """
    _call_cpp_func(_cpp.isinf, x)


def isfinite(x: _cpp.Variable) -> bool:
    """
    Returns true if all elements are finite.
    """
    _call_cpp_func(_cpp.isfinite, x)


def isposinf(x: _cpp.Variable) -> bool:
    """
    Returns true if any element is a positive infinity.
    """
    _call_cpp_func(_cpp.isposinf, x)


def isneginf(x: _cpp.Variable) -> bool:
    """
    Returns true if any element is a negative infinity.
    """
    _call_cpp_func(_cpp.isneginf, x)
