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

    Examples
    --------
    Check if values are evenly spaced:

      >>> import scipp as sc
      >>> x = sc.linspace('x', 0.0, 1.0, num=5, unit='m')
      >>> x
      <scipp.Variable> (x: 5)    float64              [m]  [0, 0.25, ..., 0.75, 1]
      >>> sc.islinspace(x)
      <scipp.Variable> ()       bool        <no unit>  True

    Non-evenly spaced values return False:

      >>> x_nonlin = sc.array(dims=['x'], values=[0.0, 1.0, 3.0, 4.0], unit='m')
      >>> sc.islinspace(x_nonlin)
      <scipp.Variable> ()       bool        <no unit>  False
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

    Examples
    --------
    Check if a 1-D variable is sorted:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1, 2, 3, 4, 5])
      >>> sc.issorted(x, 'x')
      <scipp.Variable> ()       bool        <no unit>  True

    Not sorted values return False:

      >>> x_unsorted = sc.array(dims=['x'], values=[1, 3, 2, 4, 5])
      >>> sc.issorted(x_unsorted, 'x')
      <scipp.Variable> ()       bool        <no unit>  False

    For multi-dimensional data, returns a result with one less dimension:

      >>> data_2d = sc.array(dims=['x', 'y'], values=[[1, 2, 3], [4, 5, 6]])
      >>> sc.issorted(data_2d, 'y')
      <scipp.Variable> (x: 2)       bool        <no unit>  [True, True]
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

    Examples
    --------
    Check if all values are sorted in ascending order:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1, 2, 3, 4, 5])
      >>> sc.allsorted(x, 'x')
      True

    Unsorted values return False:

      >>> x_unsorted = sc.array(dims=['x'], values=[1, 3, 2, 4, 5])
      >>> sc.allsorted(x_unsorted, 'x')
      False

    Check descending order:

      >>> x_desc = sc.array(dims=['x'], values=[5, 4, 3, 2, 1])
      >>> sc.allsorted(x_desc, 'x', order='descending')
      True
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

    Examples
    --------
    Sort a variable by its own values:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[3, 1, 4, 1, 5], unit='m')
      >>> sc.sort(x, key='x')
      <scipp.Variable> (x: 5)      int64              [m]  [1, 1, ..., 4, 5]

    Sort a DataArray by a coordinate:

      >>> da = sc.DataArray(
      ...     data=sc.array(dims=['x'], values=[10, 20, 30, 40], unit='K'),
      ...     coords={'x': sc.array(dims=['x'], values=[3.0, 1.0, 4.0, 2.0], unit='m')}
      ... )
      >>> sc.sort(da, key='x')
      <scipp.DataArray>
      Dimensions: Sizes[x:4, ]
      Coordinates:
      * x                         float64              [m]  (x)  [1, 2, 3, 4]
      Data:
                                    int64              [K]  (x)  [20, 40, 10, 30]

    Sort in descending order:

      >>> sc.sort(x, key='x', order='descending')
      <scipp.Variable> (x: 5)      int64              [m]  [5, 4, ..., 1, 1]
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

    Examples
    --------
    Extract values from a variable with variances:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], variances=[0.1, 0.2, 0.3], unit='m')
      >>> x
      <scipp.Variable> (x: 3)    float64              [m]  [1, 2, 3]  [0.1, 0.2, 0.3]
      >>> sc.values(x)
      <scipp.Variable> (x: 3)    float64              [m]  [1, 2, 3]
    """  # noqa: E501
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

    Examples
    --------
    Extract variances as values:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], variances=[0.1, 0.2, 0.3], unit='m')
      >>> sc.variances(x)
      <scipp.Variable> (x: 3)    float64            [m^2]  [0.1, 0.2, 0.3]
    """  # noqa: E501
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

    Examples
    --------
    Extract standard deviations (square root of variances) as values:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0], variances=[0.1, 0.2, 0.3], unit='m')
      >>> sc.stddevs(x)
      <scipp.Variable> (x: 3)    float64              [m]  [0.316228, 0.447214, 0.547723]
    """  # noqa: E501
    return _call_cpp_func(_cpp.stddevs, x)  # type: ignore[return-value]


@overload
def where(
    condition: Variable | DataArray, x: DataArray, y: Variable | DataArray
) -> DataArray: ...


@overload
def where(
    condition: Variable | DataArray, x: Variable | DataArray, y: DataArray
) -> DataArray: ...


@overload
def where(condition: Variable | DataArray, x: Variable, y: Variable) -> Variable: ...


def where(
    condition: Variable | DataArray, x: Variable | DataArray, y: Variable | DataArray
) -> Variable | DataArray:
    """Return elements chosen from x or y depending on condition.

    Parameters
    ----------
    condition:
        Array with dtype=bool.
    x:
        Array with values from which to choose.
    y:
        Array with values from which to choose.

    Returns
    -------
    :
        Array with elements from x where condition is True
        and elements from y elsewhere.

    Examples
    --------
    Select values based on a condition:

      >>> import scipp as sc
      >>> x = sc.array(dims=['x'], values=[1.0, 2.0, 3.0, 4.0, 5.0], unit='m')
      >>> condition = x > sc.scalar(3.0, unit='m')
      >>> condition
      <scipp.Variable> (x: 5)       bool        <no unit>  [False, False, ..., True, True]
      >>> y = sc.array(dims=['x'], values=[10.0, 20.0, 30.0, 40.0, 50.0], unit='m')
      >>> z = sc.array(dims=['x'], values=[100.0, 200.0, 300.0, 400.0, 500.0], unit='m')
      >>> sc.where(condition, y, z)
      <scipp.Variable> (x: 5)    float64              [m]  [100, 200, ..., 40, 50]

    With a DataArray, coordinates are preserved:

      >>> da1 = sc.DataArray(
      ...     data=sc.array(dims=['x'], values=[1, 2, 3], unit='K'),
      ...     coords={'x': sc.arange('x', 3, unit='m')}
      ... )
      >>> da2 = sc.DataArray(
      ...     data=sc.array(dims=['x'], values=[10, 20, 30], unit='K'),
      ...     coords={'x': sc.arange('x', 3, unit='m')}
      ... )
      >>> cond = sc.array(dims=['x'], values=[True, False, True])
      >>> sc.where(cond, da1, da2)
      <scipp.DataArray>
      Dimensions: Sizes[x:3, ]
      Coordinates:
      * x                           int64              [m]  (x)  [0, 1, 2]
      Data:
                                    int64              [K]  (x)  [1, 20, 3]
    """  # noqa: E501
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

    Examples
    --------
    Convert units only:

      >>> import scipp as sc
      >>> var = sc.array(dims=['x'], values=[1000.0, 2000.0], unit='m')
      >>> var.to(unit='km')
      <scipp.Variable> (x: 2)    float64             [km]  [1, 2]

    Convert dtype only:

      >>> var_int = sc.array(dims=['x'], values=[1, 2, 3])
      >>> var_int.to(dtype='float64')
      <scipp.Variable> (x: 3)    float64  [dimensionless]  [1, 2, 3]

    Convert both unit and dtype:

      >>> var_m = sc.array(dims=['x'], values=[1000, 2000, 3000], unit='m')
      >>> var_m.to(unit='km', dtype='float32')
      <scipp.Variable> (x: 3)    float32             [km]  [1, 2, 3]
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

    Examples
    --------
    Merge two datasets with different data items:

      >>> import scipp as sc
      >>> ds1 = sc.Dataset(data={'a': sc.scalar(1, unit='m'), 'b': sc.scalar(2, unit='s')})
      >>> ds2 = sc.Dataset(data={'c': sc.scalar(3, unit='K'), 'd': sc.scalar(4, unit='kg')})
      >>> sc.merge(ds1, ds2)
      <scipp.Dataset>
      Dimensions: Sizes[]
      Data:
        a                           int64              [m]  ()  1
        b                           int64              [s]  ()  2
        c                           int64              [K]  ()  3
        d                           int64             [kg]  ()  4

    Identical items in both inputs are allowed:

      >>> ds3 = sc.Dataset(data={'a': sc.scalar(1, unit='m'), 'e': sc.scalar(5, unit='A')})
      >>> sc.merge(ds1, ds3)
      <scipp.Dataset>
      Dimensions: Sizes[]
      Data:
        a                           int64              [m]  ()  1
        b                           int64              [s]  ()  2
        e                           int64              [A]  ()  5
    """  # noqa: E501
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
