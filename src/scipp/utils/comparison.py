# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Owen Arnold
"""
Advanced comparisons.
"""

from ..core import CoordError, DataArray, DType, Variable, all, isclose


def isnear(
    x: DataArray,
    y: DataArray,
    rtol: Variable | None = None,
    atol: Variable | None = None,
    include_data: bool = True,
    equal_nan: bool = True,
) -> bool:
    """
    Similar to scipp.isclose, but intended to compare whole DataArrays.
    Coordinates compared element by element with

    .. code-block:: python

        abs(x - y) <= atol + rtol * abs(y)

    Compared coord and attr pairs are only considered equal if all
    element-wise comparisons are True.

    See scipp.isclose for more details on how the comparisons on each
    item will be conducted.

    Parameters
    ----------
    x:
        lhs input
    y:
        rhs input
    rtol:
        relative tolerance (to y)
    atol:
        absolute tolerance
    include_data:
        Compare data element-wise between x, and y
    equal_nan:
        If ``True``, consider NaNs or infs to be equal
        providing that they match in location and, for infs,
        have the same sign

    Returns
    -------
    :
        ``True`` if near

    Raises
    ------
    Exception:
        If `x`, `y` are not going to be logically comparable
        for reasons relating to shape, item naming or non-finite elements.
    """
    same_data: bool = (
        all(isclose(x.data, y.data, rtol=rtol, atol=atol, equal_nan=equal_nan)).value
        if include_data
        else True
    )
    same_len = len(x.coords) == len(y.coords)
    if not same_len:
        return False
    for key, val in x.coords.items():
        a = x.coords[key]
        b = y.coords[key]
        if a.shape != b.shape:
            raise CoordError(
                f'Coords with key {key} have different'
                f' shapes. For x, shape is {a.shape}. For y, shape = {b.shape}'
            )
        if val.dtype in [DType.float64, DType.float32]:
            if not all(isclose(a, b, rtol=rtol, atol=atol, equal_nan=equal_nan)).value:
                return False
    return same_data
