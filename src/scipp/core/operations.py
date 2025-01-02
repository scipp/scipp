# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Matthew Andrew
from __future__ import annotations

from typing import Any, Literal, TypeVar, overload

from .._scipp import core as _cpp
from ..typing import ScippIndex, VariableLikeType
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .comparison import identical
from .cpp_classes import DataArray, Dataset, DatasetError, Variable
from .data_group import DataGroup
from .unary import to_unit

_T = TypeVar('_T', Variable, DataGroup[object])
_VarDa = TypeVar('_VarDa', Variable, DataArray)
_DsDg = TypeVar('_DsDg', Dataset, DataGroup[object])
_E1 = TypeVar('_E1')
_E2 = TypeVar('_E2')


def islinspace(x: _T, dim: str | None = None) -> _T:
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
        return _call_cpp_func(_cpp.islinspace, x)  # type: ignore[return-value]
    else:
        return _call_cpp_func(_cpp.islinspace, x, dim)  # type: ignore[return-value]


def issorted(
    x: _T, dim: str, order: Literal['ascending', 'descending'] = 'ascending'
) -> _T:
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
    return _call_cpp_func(_cpp.issorted, x, dim, order)  # type: ignore[return-value]


@overload
def allsorted(
    x: Variable, dim: str, order: Literal['ascending', 'descending'] = 'ascending'
) -> bool: ...


@overload
def allsorted(
    x: DataGroup[object],
    dim: str,
    order: Literal['ascending', 'descending'] = 'ascending',
) -> DataGroup[object]: ...


def allsorted(
    x: Variable | DataGroup[object],
    dim: str,
    order: Literal['ascending', 'descending'] = 'ascending',
) -> bool | DataGroup[object]:
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


def sort(
    x: VariableLikeType,
    key: str | Variable,
    order: Literal['ascending', 'descending'] = 'ascending',
) -> VariableLikeType:
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
    return _call_cpp_func(_cpp.sort, x, key, order)  # type: ignore[return-value]


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
    return _call_cpp_func(_cpp.values, x)  # type: ignore[return-value]


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
    return _call_cpp_func(_cpp.variances, x)  # type: ignore[return-value]


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
    return _call_cpp_func(_cpp.stddevs, x)  # type: ignore[return-value]


def where(condition: Variable, x: Variable, y: Variable) -> Variable:
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
    """
    return _call_cpp_func(_cpp.where, condition, x, y)  # type: ignore[return-value]


def to(
    var: Variable,
    *,
    unit: _cpp.Unit | str | None = None,
    dtype: Any | None = None,
    copy: bool = True,
) -> Variable:
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


def merge(lhs: _DsDg, rhs: _DsDg) -> _DsDg:
    """Merge two datasets or data groups into one.

    If an item appears in both inputs, it must have an identical value in both.

    Parameters
    ----------
    lhs: Dataset | DataGroup
        First dataset or data group.
    rhs: Dataset | DataGroup
        Second dataset or data group.

    Returns
    -------
    : Dataset | DataGroup
        A new object that contains the union of all data items,
        coords, masks and attributes.

    Raises
    ------
    scipp.DatasetError
        If there are conflicting items with different content.
    """
    # Check both arguments to make `_cpp.merge` raise TypeError on mismatch.
    if isinstance(lhs, Dataset) or isinstance(rhs, Dataset):  # type: ignore[redundant-expr]
        return _call_cpp_func(_cpp.merge, lhs, rhs)  # type: ignore[return-value]
    return _merge_data_group(lhs, rhs)


def _generic_identical(a: Any, b: Any) -> bool:
    try:
        return identical(a, b)
    except TypeError:
        from numpy import array_equal

        try:
            return array_equal(a, b)
        except TypeError:
            return a == b  # type: ignore[no-any-return]


def _merge_data_group(lhs: DataGroup[_E1], rhs: DataGroup[_E2]) -> DataGroup[_E1 | _E2]:
    res: DataGroup[_E1 | _E2] = DataGroup(dict(lhs))
    for k, v in rhs.items():
        if k in res and not _generic_identical(res[k], v):
            raise DatasetError(f"Cannot merge data groups. Mismatch in item {k}")
        res[k] = v
    return res


def _raw_positional_index(
    sizes: dict[str, int], coord: Variable, index: slice | Variable
) -> tuple[str, list[int | Variable]]:
    dim, *inds = _call_cpp_func(
        _cpp.label_based_index_to_positional_index,
        list(sizes.keys()),
        list(sizes.values()),
        coord,
        index,
    )
    return dim, inds  # type: ignore[return-value]


def label_based_index_to_positional_index(
    sizes: dict[str, int],
    coord: Variable,
    index: slice | Variable,
) -> ScippIndex:
    """Returns the positional index equivalent to label based indexing
    the coord with values."""
    dim, inds = _raw_positional_index(sizes, coord, index)
    # Length of inds is 1 if index was a variable
    if len(inds) == 1:
        if inds[0] < 0 or sizes[dim] <= inds[0]:
            # If index is a variable and the coord is a bin-edge coord
            #   - if the index is less than any edge then inds[0] is -1
            #   - if the index is greater than any edge then inds[0] is sizes[dim]
            raise IndexError(
                f"Value {index} is not contained in the bin-edge coord {coord}"
            )
        return (dim, inds[0])
    return (dim, slice(*inds))


def as_const(x: _VarDa) -> _VarDa:
    """Return a copy with the readonly flag set."""
    return _call_cpp_func(_cpp.as_const, x)  # type: ignore[return-value]
