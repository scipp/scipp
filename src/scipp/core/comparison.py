# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
# flake8: noqa: E501

from __future__ import annotations

from .._scipp import core as _cpp
from ..typing import VariableLike
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .variable import scalar


def less(x: VariableLike, y: VariableLike) -> VariableLike:
    """Element-wise '<' (less).

    Warning: If one or both of the operators have variances (uncertainties)
    they are ignored silently, i.e., comparison is based exclusively on
    the values.

    Parameters
    ----------
    x:
        Left input.
    y:
        Right input.

    Returns
    -------
    :
        Booleans that are true where `a < b`.
    """
    return _call_cpp_func(_cpp.less, x, y)


def greater(x: VariableLike, y: VariableLike) -> VariableLike:
    """Element-wise '>' (greater).

    Warning: If one or both of the operators have variances (uncertainties)
    they are ignored silently, i.e., comparison is based exclusively on
    the values.

    Parameters
    ----------
    x:
        Left input.
    y:
        Right input.

    Returns
    -------
    :
        Booleans that are true where `a > b`.
    """
    return _call_cpp_func(_cpp.greater, x, y)


def less_equal(x: VariableLike, y: VariableLike) -> VariableLike:
    """Element-wise '<=' (less_equal).

    Warning: If one or both of the operators have variances (uncertainties)
    they are ignored silently, i.e., comparison is based exclusively on
    the values.

    Parameters
    ----------
    x:
        Left input.
    y:
        Right input.

    Returns
    -------
    :
        Booleans that are true where `a <= b`.
    """
    return _call_cpp_func(_cpp.less_equal, x, y)


def greater_equal(x: VariableLike, y: VariableLike) -> VariableLike:
    """Element-wise '>=' (greater_equal).

    Warning: If one or both of the operators have variances (uncertainties)
    they are ignored silently, i.e., comparison is based exclusively on
    the values.

    Parameters
    ----------
    x:
        Left input.
    y:
        Right input.

    Returns
    -------
    :
        Booleans that are true where `a >= b`.
    """
    return _call_cpp_func(_cpp.greater_equal, x, y)


def equal(x: VariableLike, y: VariableLike) -> VariableLike:
    """Element-wise '==' (equal).

    Warning: If one or both of the operators have variances (uncertainties)
    they are ignored silently, i.e., comparison is based exclusively on
    the values.

    Parameters
    ----------
    x:
        Left input.
    y:
        Right input.

    Returns
    -------
    :
        Booleans that are true where `a == b`.
    """
    return _call_cpp_func(_cpp.equal, x, y)


def not_equal(x: VariableLike, y: VariableLike) -> VariableLike:
    """Element-wise '!=' (not_equal).

    Warning: If one or both of the operators have variances (uncertainties)
    they are ignored silently, i.e., comparison is based exclusively on
    the values.

    Parameters
    ----------
    x:
        Left input.
    y:
        Right input.

    Returns
    -------
    :
        Booleans that are true where `a != b`.
    """
    return _call_cpp_func(_cpp.not_equal, x, y)


def identical(x: VariableLike,
              y: VariableLike,
              *,
              equal_nan: bool = False) -> VariableLike:
    """Full comparison of x and y.

    Parameters
    ----------
    x:
        Left input.
    y:
        Right input.
    equal_nan:
        If true, non-finite values at the same index in (x, y) are treated as equal.
        Signbit must match for infs.

    Returns
    -------
    :
        True if x and y have identical values, variances, dtypes, units,
        dims, shapes, coords, and masks. Else False.
    """
    return _call_cpp_func(_cpp.identical, x, y, equal_nan=equal_nan)


def isclose(x: _cpp.Variable,
            y: _cpp.Variable,
            *,
            rtol: _cpp.Variable = None,
            atol: _cpp.Variable = None,
            equal_nan: bool = False) -> _cpp.Variable:
    """Checks element-wise if the inputs are close to each other.

    Compares values of x and y element by element against absolute
    and relative tolerances according to (non-symmetric)

    .. code-block:: python

        abs(x - y) <= atol + rtol * abs(y)

    If both x and y have variances, the variances are also compared
    between elements. In this case, both values and variances must
    be within the computed tolerance limits. That is:

    .. code-block:: python

        abs(x.value - y.value) <= atol + rtol * abs(y.value) and
          abs(sqrt(x.variance) - sqrt(y.variance)) <= atol + rtol * abs(sqrt(y.variance))

    Attention
    ---------
        Vectors and matrices are compared element-wise.
        This is not necessarily a good measure for the similarity of `spatial`
        dtypes like ``scipp.DType.rotation3`` or ``scipp.Dtype.affine_transform3``
        (see :mod:`scipp.spatial`).

    Parameters
    ----------
    x:
        Left input.
    y:
        Right input.
    rtol:
        Tolerance value relative (to y).
        Can be a scalar or non-scalar.
        Defaults to scalar 1e-5 if unset.
    atol:
        Tolerance value absolute.
        Can be a scalar or non-scalar.
        Defaults to scalar 1e-8 if unset and takes units from y arg.
    equal_nan:
        If true, non-finite values at the same index in (x, y) are treated as equal.
        Signbit must match for infs.

    Returns
    -------
    :
        Variable same size as input.
        Element True if absolute diff of value <= atol + rtol * abs(y),
        otherwise False.

    See Also
    --------
    scipp.allclose:
        Equivalent of ``sc.all(sc.isclose(...)).value``.
    """
    if atol is None:
        atol = scalar(1e-8, unit=y.unit)
    if rtol is None:
        rtol = scalar(1e-5, unit=None if atol.unit is None else _cpp.units.one)
    return _call_cpp_func(_cpp.isclose, x, y, rtol, atol, equal_nan)


def allclose(x, y, rtol=None, atol=None, equal_nan=False) -> True:
    """Checks if all elements in the inputs are close to each other.

    Verifies that ALL element-wise comparisons meet the condition:

    abs(x - y) <= atol + rtol * abs(y)

    If both x and y have variances, the variances are also compared
    between elements. In this case, both values and variances must
    be within the computed tolerance limits. That is:

    .. code-block:: python

        abs(x.value - y.value) <= atol + rtol * abs(y.value) and abs(
            sqrt(x.variance) - sqrt(y.variance)) \
                <= atol + rtol * abs(sqrt(y.variance))

    Attention
    ---------
        Vectors and matrices are compared element-wise.
        This is not necessarily a good measure for the similarity of `spatial`
        dtypes like ``scipp.DType.rotation3`` or ``scipp.Dtype.affine_transform3``
        (see :mod:`scipp.spatial`).

    Parameters
    ----------
    x:
        Left input.
    y:
        Right input.
    rtol:
        Tolerance value relative (to y).
        Can be a scalar or non-scalar.
        Cannot have variances.
        Defaults to scalar 1e-5 if unset.
    atol:
        Tolerance value absolute.
        Can be a scalar or non-scalar.
        Cannot have variances.
        Defaults to scalar 1e-8 if unset and takes units from y arg.
    equal_nan:
        If true, non-finite values at the same index in (x, y) are treated as equal.
        Signbit must match for infs.

    Returns
    -------
    :
        True if for all elements ``value <= atol + rtol * abs(y)``, otherwise False.

    See Also
    --------
    scipp.isclose:
        Compares element-wise with specified tolerances.
    """
    return _call_cpp_func(_cpp.all,
                          isclose(x, y, rtol=rtol, atol=atol,
                                  equal_nan=equal_nan)).value
