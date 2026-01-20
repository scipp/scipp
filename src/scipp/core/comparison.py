# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
# ruff: noqa: E501

from __future__ import annotations

from typing import Any

import numpy as np

from .._scipp import core as _cpp
from ..typing import VariableLike
from . import data_group
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .cpp_classes import DataArray, Dataset, Variable
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

    Examples
    --------
    Compare an array with a scalar:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> sc.less(x, sc.scalar(2.0, unit='m'))
      <scipp.Variable> (x: 3)       bool        <no unit>  [True, False, False]

    Compare two arrays element-wise:

      >>> y = sc.array(dims=['x'], values=[2.0, 2.0, 2.0], unit='m')
      >>> sc.less(x, y)
      <scipp.Variable> (x: 3)       bool        <no unit>  [True, False, False]

    Variances are ignored in comparisons (only values are compared):

      >>> a = sc.array(dims=['x'], values=[1.0, 2.0], variances=[0.1, 0.1])
      >>> b = sc.array(dims=['x'], values=[1.0, 2.0], variances=[999.0, 999.0])
      >>> sc.less(a, b)  # Same values, different variances -> all False
      <scipp.Variable> (x: 2)       bool        <no unit>  [False, False]
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

    Examples
    --------
    Compare an array with a scalar:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> sc.greater(x, sc.scalar(2.0, unit='m'))
      <scipp.Variable> (x: 3)       bool        <no unit>  [False, False, True]
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

    Examples
    --------
    Compare an array with a scalar:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> sc.less_equal(x, sc.scalar(2.0, unit='m'))
      <scipp.Variable> (x: 3)       bool        <no unit>  [True, True, False]
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

    Examples
    --------
    Compare an array with a scalar:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> sc.greater_equal(x, sc.scalar(2.0, unit='m'))
      <scipp.Variable> (x: 3)       bool        <no unit>  [False, True, True]
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

    Examples
    --------
    Compare an array with a scalar:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> sc.equal(x, sc.scalar(2.0, unit='m'))
      <scipp.Variable> (x: 3)       bool        <no unit>  [False, True, False]
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

    Examples
    --------
    Compare an array with a scalar:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> sc.not_equal(x, sc.scalar(2.0, unit='m'))
      <scipp.Variable> (x: 3)       bool        <no unit>  [True, False, True]
    """
    return _call_cpp_func(_cpp.not_equal, x, y)


def _identical_data_groups(
    x: data_group.DataGroup[Any], y: data_group.DataGroup[Any], *, equal_nan: bool
) -> bool:
    def compare(a: Any, b: Any) -> bool:
        if not isinstance(a, type(b)):
            return False
        if isinstance(a, Variable | DataArray | Dataset | data_group.DataGroup):
            return identical(a, b, equal_nan=equal_nan)
        if isinstance(a, np.ndarray):
            return np.array_equal(a, b, equal_nan=equal_nan)
        # Explicit conversion to bool in case __eq__ returns
        # something else like an array.
        return bool(a == b)

    if x.keys() != y.keys():
        return False
    return all(compare(x[k], y[k]) for k in x.keys())


def identical(x: VariableLike, y: VariableLike, *, equal_nan: bool = False) -> bool:
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

    Examples
    --------
    Compare two identical arrays:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> y = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> sc.identical(x, y)
      True

    Arrays with different units are not identical:

      >>> z = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='cm')
      >>> sc.identical(x, z)
      False
    """
    if isinstance(x, data_group.DataGroup):
        if not isinstance(y, data_group.DataGroup):
            raise TypeError("Both or neither of the arguments must be a DataGroup")
        return _identical_data_groups(x, y, equal_nan=equal_nan)

    return _call_cpp_func(_cpp.identical, x, y, equal_nan=equal_nan)  # type: ignore[return-value]


def isclose(
    x: _cpp.Variable,
    y: _cpp.Variable,
    *,
    rtol: _cpp.Variable = None,
    atol: _cpp.Variable = None,
    equal_nan: bool = False,
) -> _cpp.Variable:
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

    Examples
    --------
    Compare with default tolerances:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> y = sc.array(dims=['x'], values=[1.0001, 2.0, 2.999], unit='m')
      >>> sc.isclose(x, y)
      <scipp.Variable> (x: 3)       bool        <no unit>  [False, True, False]

    With custom tolerances:

      >>> sc.isclose(x, y, rtol=sc.scalar(1e-3), atol=sc.scalar(1e-3, unit='m'))
      <scipp.Variable> (x: 3)       bool        <no unit>  [True, True, True]
    """
    if atol is None:
        atol = scalar(1e-8, unit=y.unit)
    if rtol is None:
        rtol = scalar(1e-5, unit=None if atol.unit is None else _cpp.units.one)
    return _call_cpp_func(_cpp.isclose, x, y, rtol, atol, equal_nan)


def allclose(
    x: _cpp.Variable,
    y: _cpp.Variable,
    rtol: _cpp.Variable = None,
    atol: _cpp.Variable = None,
    equal_nan: bool = False,
) -> bool:
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

    Examples
    --------
    Arrays that are close within default tolerances:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], unit='m')
      >>> y = sc.array(dims=['x'], values=[1.0001, 2.0, 3.0001], unit='m')
      >>> sc.allclose(x, y)
      np.False_

    Arrays with larger differences:

      >>> z = sc.array(dims=['x'], values=[1.1, 2.0, 3.0], unit='m')
      >>> sc.allclose(x, z)
      np.False_
    """
    return _call_cpp_func(  # type:ignore[no-any-return]
        _cpp.all, isclose(x, y, rtol=rtol, atol=atol, equal_nan=equal_nan)
    ).value  # type: ignore[union-attr]
