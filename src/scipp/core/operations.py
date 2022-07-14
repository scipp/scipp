# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from __future__ import annotations
from typing import Any, Literal, Optional, TypeVar, Union

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .unary import to_unit
from ..typing import VariableLikeType

_ContainerWithCoords = TypeVar('_ContainerWithCoords', _cpp.DataArray, _cpp.Dataset)


def islinspace(x: _cpp.Variable, dim: str = None) -> _cpp.Variable:
    """Check if the values of a variable are evenly spaced.

    Parameters
    ----------
    x:
        Variable to check.
    dim:
        Optional variable for the dim to check from the Variable.

    Returns
    -------
    :
        Variable of value True if the variable contains regularly
        spaced values, variable of value False otherwise.
    """
    if dim is None:
        return _call_cpp_func(_cpp.islinspace, x)
    else:
        return _call_cpp_func(_cpp.islinspace, x, dim)


def issorted(x: _cpp.Variable,
             dim: str,
             order: Literal['ascending', 'descending'] = 'ascending') -> _cpp.Variable:
    """Check if the values of a variable are sorted.

    - If ``order`` is 'ascending',
      check if values are non-decreasing along ``dim``.
    - If ``order`` is 'descending',
      check if values are non-increasing along ``dim``.

    Parameters
    ----------
    x:
        Variable to check.
    dim:
        Dimension along which order is checked.
    order:
        Sorting order.

    Returns
    -------
    :
        Variable containing one less dim than the original
        variable with the corresponding boolean value for whether
        it was sorted along the given dim for the other dimensions.

    See Also
    --------
    scipp.allsorted
    """
    return _call_cpp_func(_cpp.issorted, x, dim, order)


def allsorted(x: _cpp.Variable,
              dim: str,
              order: Literal['ascending', 'descending'] = 'ascending') -> bool:
    """Check if all values of a variable are sorted.

    - If ``order`` is 'ascending',
      check if values are non-decreasing along ``dim``.
    - If ``order`` is 'descending',
      check if values are non-increasing along ``dim``.

    Parameters
    ----------
    x:
        Variable to check.
    dim:
        Dimension along which order is checked.
    order:
        Sorting order.

    Returns
    -------
    :
        True if the variable values are monotonously ascending or
        descending (depending on the requested order), False otherwise.

    See Also
    --------
    scipp.issorted
    """
    return _call_cpp_func(_cpp.allsorted, x, dim, order)


def sort(x: VariableLikeType,
         key: Union[str, _cpp.Variable],
         order: Literal['ascending', 'descending'] = 'ascending') -> VariableLikeType:
    """Sort variable along a dimension by a sort key or dimension label.

    - If ``order`` is 'ascending',
      sort such that values are non-decreasing according to ``key``.
    - If ``order`` is 'descending',
      sort such that values are non-increasing according to ``key``.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Data to be sorted.
    key:
        Either a 1D variable sort key or a dimension label.
    order:
        Sorting order.

    Returns
    -------
    : scipp.typing.VariableLike
        The sorted equivalent of the input with the same type.

    Raises
    ------
    scipp.DimensionError
        If the key is a Variable that does not have exactly 1 dimension.
    """
    return _call_cpp_func(_cpp.sort, x, key, order)


def values(x: VariableLikeType) -> VariableLikeType:
    """Return the input without variances.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data.

    Returns
    -------
    : scipp.typing.VariableLike
        The same as the input but without variances.

    See Also
    --------
    scipp.variances, scipp.stddevs
    """
    return _call_cpp_func(_cpp.values, x)


def variances(x: VariableLikeType) -> VariableLikeType:
    """Return the input's variances as values.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data with variances.

    Returns
    -------
    : scipp.typing.VariableLike
        The same as the input but with values set to the input's variances
        and without variances itself.

    See Also
    --------
    scipp.values, scipp.stddevs
    """
    return _call_cpp_func(_cpp.variances, x)


def stddevs(x: VariableLikeType) -> VariableLikeType:
    """Return the input's standard deviations as values.

    Parameters
    ----------
    x: scipp.typing.VariableLike
        Input data with variances.

    Returns
    -------
    : scipp.typing.VariableLike
        The same as the input but with values set to standard deviations computed
        from the input's variances and without variances itself.

    See Also
    --------
    scipp.values, scipp.variances
    """
    return _call_cpp_func(_cpp.stddevs, x)


def where(condition: _cpp.Variable, x: _cpp.Variable,
          y: _cpp.Variable) -> _cpp.Variable:
    """Return elements chosen from x or y depending on condition.

    Parameters
    ----------
    condition:
        Variable with dtype=bool.
    x:
        Variable with values from which to choose.
    y:
        Variable with values from which to choose.

    Returns
    -------
    :
        Variable with elements from x where condition is True
        and elements from y elsewhere.

    See Also
    --------
    scipp.choose
    """
    return _call_cpp_func(_cpp.where, condition, x, y)


def to(
        var: VariableLikeType,  # noqa
        *,
        unit: Optional[Union[_cpp.Unit, str]] = None,
        dtype: Optional[Any] = None,
        copy: bool = True) -> VariableLikeType:
    """Converts a Variable or DataArray to a different dtype and/or a different unit.

    If the dtype and unit are both unchanged and ``copy`` is `False`,
    the object is returned without making a deep copy.

    This method will choose whether to do the dtype or units translation first, by
    using the following rules in order:

    - If either the input or output dtype is float64, the unit translation will be done
      on the float64 type
    - If either the input or output dtype is float32, the unit translation will be done
      on the float32 type
    - If both the input and output dtypes are integer types, the unit translation will
      be done on the larger type
    - In other cases, the dtype is converted first and then the unit translation is done

    Parameters
    ----------
    unit:
        Target unit. If ``None``, the unit is unchanged.
    dtype:
        Target dtype. If ``None``, the dtype is unchanged.
    copy:
        If ``False``, return the input object if possible.
        If ``True``, the function always returns a new object.

    Returns
    -------
    : Same as input
        New object with specified dtype and unit.

    Raises
    ------
    scipp.DTypeError
        If the input cannot be converted to the given dtype.
    scipp.UnitError
        If the input cannot be converted to the given unit.

    See Also
    --------
    scipp.to_unit, scipp.DataArray.astype, scipp.Variable.astype
    """
    if unit is None and dtype is None:
        raise ValueError("Must provide dtype or unit or both")

    if dtype is None:
        return to_unit(var, unit, copy=copy)

    if unit is None:
        return var.astype(dtype, copy=copy)

    if dtype == _cpp.DType.float64:
        convert_dtype_first = True
    elif var.dtype == _cpp.DType.float64:
        convert_dtype_first = False
    elif dtype == _cpp.DType.float32:
        convert_dtype_first = True
    elif var.dtype == _cpp.DType.float32:
        convert_dtype_first = False
    elif var.dtype == _cpp.DType.int64 and dtype == _cpp.DType.int32:
        convert_dtype_first = False
    elif var.dtype == _cpp.DType.int32 and dtype == _cpp.DType.int64:
        convert_dtype_first = True
    else:
        convert_dtype_first = True

    if convert_dtype_first:
        return to_unit(var.astype(dtype, copy=copy), unit=unit, copy=False)
    else:
        return to_unit(var, unit=unit, copy=copy).astype(dtype, copy=False)
