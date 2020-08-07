# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def sin(x, out=None):
    """Element-wise sin.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not a plane-angle unit, or if the sin cannot be
             computed on the dtype, e.g., if it is an integer.
    :return: The sin values of the input.
    """
    return _call_cpp_func(_cpp.sin, x, out=out)


def cos(x, out=None):
    """Element-wise cos.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not a plane-angle unit, or if the cos cannot be
             computed on the dtype, e.g., if it is an integer.
    :return: The cos values of the input.
    """
    return _call_cpp_func(_cpp.cos, x, out=out)


def tan(x, out=None):
    """Element-wise tan.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not a plane-angle unit, or if the tan cannot be
             computed on the dtype, e.g., if it is an integer.
    :return: The tan values of the input.
    """
    return _call_cpp_func(_cpp.tan, x, out=out)


def asin(x, out=None):
    """Element-wise asin.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not dimensionless.
    :return: The asin values of the input.
    """
    return _call_cpp_func(_cpp.asin, x, out=out)


def acos(x, out=None):
    """Element-wise acos.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not dimensionless.
    :return: The acos values of the input.
    """
    return _call_cpp_func(_cpp.acos, x, out=out)


def atan(x, out=None):
    """Element-wise atan.

    :param x: Input data.
    :param out: Optional output buffer.
    :raises: If the unit is not dimensionless.
    :return: The atan values of the input.
    """
    return _call_cpp_func(_cpp.atan, x, out=out)


def atan2(y, x, out=None):
    """Element-wise atan2.

    :param y: Input y values.
    :param x: Input x values.
    :param out: Optional output buffer.
    :return: The atan2 values of the inputs.
    """
    return _call_cpp_func(_cpp.atan, x, out=out)
