# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import Literal, TypeVar

from .argument_handlers import IntoStrDict, combine_dict_args
from .cpp_classes import DataArray, Dataset, Variable

_T = TypeVar('_T', Dataset, DataArray)


def _assign(
    obj: _T,
    name: Literal['coords', 'masks'],
    obj_attrs: IntoStrDict[Variable] | None = None,
    /,
    **kw_obj_attrs: Variable,
) -> _T:
    out = obj.copy(deep=False)
    collected = combine_dict_args(obj_attrs, kw_obj_attrs)
    for key, value in collected.items():
        getattr(out, name)[key] = value
    return out


def assign_data(self: DataArray, data: Variable) -> DataArray:
    """Return new data array with updated data.

    Parameters
    ----------
    data:
        New data.

    Returns
    -------
    :
        ``scipp.DataArray`` with updated data.

    Examples
    --------
    Replace the data in a DataArray while preserving coordinates:

      >>> import scipp as sc
      >>> da = sc.DataArray(
      ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0]),
      ...     coords={'x': sc.arange('x', 3)}
      ... )
      >>> new_data = sc.array(dims=['x'], values=[10.0, 20.0, 30.0], unit='m')
      >>> da.assign(new_data)
      <scipp.DataArray>
      Dimensions: Sizes[x:3, ]
      Coordinates:
      * x                           int64  [dimensionless]  (x)  [0, 1, 2]
      Data:
                                  float64              [m]  (x)  [10, 20, 30]
    """
    out = self.copy(deep=False)
    out.data = data
    return out


def assign_coords(
    self: _T, coords: IntoStrDict[Variable] | None = None, /, **coords_kwargs: Variable
) -> _T:
    """Return new object with updated or inserted coordinate.

    Parameters
    ----------
    coords :
        New coordinates.

    coords_kwargs :
        Keyword arguments form of ``coords``.

    Returns
    -------
    :
        ``scipp.DataArray`` or ``scipp.Dataset`` with updated coordinates.

    Examples
    --------
    Add a new coordinate using keyword arguments:

      >>> import scipp as sc
      >>> da = sc.DataArray(
      ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0]),
      ...     coords={'x': sc.arange('x', 3)}
      ... )
      >>> da.assign_coords(y=sc.array(dims=['x'], values=[10, 20, 30]))
      <scipp.DataArray>
      Dimensions: Sizes[x:3, ]
      Coordinates:
      * x                           int64  [dimensionless]  (x)  [0, 1, 2]
      * y                           int64  [dimensionless]  (x)  [10, 20, 30]
      Data:
                                  float64  [dimensionless]  (x)  [1, 2, 3]

    Update an existing coordinate using a dict:

      >>> da.assign_coords({'x': sc.arange('x', 3.0, unit='m')})
      <scipp.DataArray>
      Dimensions: Sizes[x:3, ]
      Coordinates:
      * x                         float64              [m]  (x)  [0, 1, 2]
      Data:
                                  float64  [dimensionless]  (x)  [1, 2, 3]

    """
    return _assign(self, 'coords', coords, **coords_kwargs)


def assign_masks(
    self: DataArray,
    masks: IntoStrDict[Variable] | None = None,
    /,
    **masks_kwargs: Variable,
) -> DataArray:
    """Return new object with updated or inserted masks.

    Parameters
    ----------
    masks :
        New masks.

    masks_kwargs :
        Keyword arguments form of ``masks``.

    Returns
    -------
    :
        ``scipp.DataArray`` with updated masks.

    Examples
    --------
    Add a new mask using keyword arguments:

      >>> import scipp as sc
      >>> da = sc.DataArray(
      ...     sc.array(dims=['x'], values=[1.0, 2.0, 3.0]),
      ...     coords={'x': sc.arange('x', 3)}
      ... )
      >>> da.assign_masks(mask=sc.array(dims=['x'], values=[False, True, False]))
      <scipp.DataArray>
      Dimensions: Sizes[x:3, ]
      Coordinates:
      * x                           int64  [dimensionless]  (x)  [0, 1, 2]
      Data:
                                  float64  [dimensionless]  (x)  [1, 2, 3]
      Masks:
        mask                         bool        <no unit>  (x)  [False, True, False]

    Add multiple masks using a dict:

      >>> da.assign_masks({
      ...     'low': da.data < sc.scalar(2.0),
      ...     'high': da.data > sc.scalar(2.0),
      ... })
      <scipp.DataArray>
      Dimensions: Sizes[x:3, ]
      Coordinates:
      * x                           int64  [dimensionless]  (x)  [0, 1, 2]
      Data:
                                  float64  [dimensionless]  (x)  [1, 2, 3]
      Masks:
        high                         bool        <no unit>  (x)  [False, False, True]
        low                          bool        <no unit>  (x)  [True, False, False]

    """
    return _assign(self, 'masks', masks, **masks_kwargs)
