# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


def less(x, y):
    """Element-wise '<' (less).

    Warning: If one or both of the operators have variances (uncertainties)
    there are ignored silently, i.e., comparison is based exclusively on
    the values.

    :param x: Left input.
    :param y: Right input.
    :raises: If the units of inputs are not the same, or if the dtypes of
             inputs cannot be compared.
    :return: Booleans that are true if `a < b`.
    """
    return _call_cpp_func(_cpp.less, x, y)


def greater(x, y):
    """Element-wise '>' (greater).

    Warning: If one or both of the operators have variances (uncertainties)
    there are ignored silently, i.e., comparison is based exclusively on
    the values.

    :param x: Left input.
    :param y: Right input.
    :raises: If the units of inputs are not the same, or if the dtypes of
             inputs cannot be compared.
    :return: Booleans that are true if `a > b`.
    """
    return _call_cpp_func(_cpp.greater, x, y)


def less_equal(x, y):
    """Element-wise '<=' (less_equal).

    Warning: If one or both of the operators have variances (uncertainties)
    there are ignored silently, i.e., comparison is based exclusively on
    the values.

    :param x: Left input.
    :param y: Right input.
    :raises: If the units of inputs are not the same, or if the dtypes of
             inputs cannot be compared.
    :return: Booleans that are true if `a <= b`.
    """
    return _call_cpp_func(_cpp.less_equal, x, y)


def greater_equal(x, y):
    """Element-wise '>=' (greater_equal).

    Warning: If one or both of the operators have variances (uncertainties)
    there are ignored silently, i.e., comparison is based exclusively on
    the values.

    :param x: Left input.
    :param y: Right input.
    :raises: If the units of inputs are not the same, or if the dtypes of
             inputs cannot be compared.
    :return: Booleans that are true if `a >= b`.
    """
    return _call_cpp_func(_cpp.greater_equal, x, y)


def equal(x, y):
    """Element-wise '==' (equal).

    Warning: If one or both of the operators have variances (uncertainties)
    there are ignored silently, i.e., comparison is based exclusively on
    the values.

    :param x: Left input.
    :param y: Right input.
    :raises: If the units of inputs are not the same, or if the dtypes of
             inputs cannot be compared.
    :return: Booleans that are true if `a == b`.
    """
    return _call_cpp_func(_cpp.equal, x, y)


def not_equal(x, y):
    """Element-wise '!=' (not_equal).

    Warning: If one or both of the operators have variances (uncertainties)
    there are ignored silently, i.e., comparison is based exclusively on
    the values.

    :param x: Left input.
    :param y: Right input.
    :raises: If the units of inputs are not the same, or if the dtypes of
             inputs cannot be compared.
    :return: Booleans that are true if `a != b`.
    """
    return _call_cpp_func(_cpp.not_equal, x, y)


def is_equal(x, y):
    """Full comparison of x and y.

    :param x: Left input.
    :param y: Right input.
    :return: True if x and y have identical values, variances, dtypes, units,
             dims, shapes, coords, and masks. Else False.
    """
    return _call_cpp_func(_cpp.is_equal, x, y)


def is_approx(x, y, tol):
    """Compares values (x, y) element by element against tolerance (tol).
    Variances are not accounted for.

    :param x: Left input.
    :param y: Right input.
    :param tol: Tolerance value.
    :return: Variable same size as input.
             Element True if absolute diff of value <= input tolerance,
             otherwise False.
    """
    return _call_cpp_func(_cpp.is_approx, x, y, tol)
