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


def is_approx(x, y, rtol, atol, equal_nan=False):
    """
    Deprecated, use isclose

    Compares values (x, y) element by element against tolerance absolute
    and relative tolerances (non-symmetric).

    abs(x - y) <= atol + rtol * y

    Variances are not accounted for.

    :param x: Left input.
    :param y: Right input.
    :param rtol: Tolerance value relative (to y).
    :param atol: Tolerance value absolute.
    :param equal_nan: if true, non-finite values at the same index in (x, y)
           are treated as equal.
           Signbit must match for infs.
    :return: Variable same size as input.
             Element True if absolute diff of value <= atol + rtol * y,
             otherwise False.
    """
    raise RuntimeError("is_approx is deprecated. Use isclose")


def is_close(x, y, rtol=None, atol=None, equal_nan=False):
    """Compares values (x, y) element by element against tolerance absolute
    and relative tolerances (non-symmetric).

    abs(x - y) <= atol + rtol * y

    Variances are not accounted for.

    :param x: Left input.
    :param y: Right input.
    :param rtol: Tolerance value relative (to y).
                 Can be a scalar or non-scalar.
                 Defaults to scalar 1e-5 if unset.
    :param atol: Tolerance value absolute. Can be a scalar or non-scalar.
                 Defaults to scalar 1e-8 if unset and takes units from y arg.
    :param equal_nan: if true, non-finite values at the same index in (x, y)
                      are treated as equal.
                      Signbit must match for infs.
    :return: Variable same size as input.
             Element True if absolute diff of value <= atol + rtol * y,
             otherwise False.
    """
    if rtol is None:
        rtol = 1e-5
    if atol is None:
        atol = 1e-8
    if not hasattr(rtol, 'unit'):
        rtol = rtol * _cpp.units.one
    if not hasattr(atol, 'unit'):
        atol = atol * y.unit
    return _call_cpp_func(_cpp.is_close, x, y, rtol, atol, equal_nan)
