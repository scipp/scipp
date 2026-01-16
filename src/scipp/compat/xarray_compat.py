# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import TYPE_CHECKING, overload
from warnings import warn

from ..core import DataArray, Dataset, Unit, Variable
from ..typing import VariableLike
from ..units import default_unit

if TYPE_CHECKING:
    import xarray as xr


@overload
def from_xarray(obj: xr.Variable) -> Variable: ...


@overload
def from_xarray(obj: xr.DataArray) -> DataArray: ...


@overload
def from_xarray(obj: xr.Dataset) -> Dataset: ...


def from_xarray(obj: xr.Variable | xr.DataArray | xr.Dataset) -> VariableLike:
    """Convert an xarray object to the corresponding scipp object.
    Attributes named `"units"` are used to set the units of the Variables.
    All other DataArray attributes are kept, but attributes of Variables, Coordinates
    and Datasets are dropped.

    Parameters
    ----------
    obj:
        The xarray object to convert.

    Returns
    -------
    :
        The converted scipp object.

    See Also
    --------
    scipp.compat.to_xarray

    Examples
    --------
    Convert an xarray DataArray to a scipp DataArray:

      >>> import scipp as sc
      >>> import xarray as xr
      >>> xr_da = xr.DataArray(
      ...     [[1, 2, 3], [4, 5, 6]],
      ...     dims=['x', 'y'],
      ...     coords={'x': [10, 20], 'y': [0.1, 0.2, 0.3]},
      ...     attrs={'units': 'm'}
      ... )
      >>> sc.compat.from_xarray(xr_da)  # doctest: +SKIP
      <scipp.DataArray>
      Dimensions: Sizes[x:2, y:3, ]
      Coordinates:
      * x                           int64  [dimensionless]  (x)  [10, 20]
      * y                         float64  [dimensionless]  (y)  [0.1, 0.2, 0.3]
      Data:
                                    int64              [m]  (x, y)  [1, 2, ..., 5, 6]
    """
    import xarray as xr

    if isinstance(obj, xr.Variable):
        return _from_xarray_variable(obj)
    elif isinstance(obj, xr.DataArray):
        return _from_xarray_dataarray(obj)
    elif isinstance(obj, xr.Dataset):
        return _from_xarray_dataset(obj)
    else:
        raise ValueError(f"from_xarray: cannot convert type '{type(obj)}'")


@overload
def to_xarray(obj: Variable) -> xr.Variable: ...


@overload
def to_xarray(obj: DataArray) -> xr.DataArray: ...


@overload
def to_xarray(obj: Dataset) -> xr.Dataset: ...


def to_xarray(obj: VariableLike) -> xr.Variable | xr.DataArray | xr.Dataset:
    """Convert a scipp object to the corresponding xarray object.

    Warning
    -------
    Any masks and variances in the input will be stripped during the conversion.
    Binned data is not supported.

    Parameters
    ----------
    obj:
        The scipp object to convert.

    Returns
    -------
    :
        The converted xarray object.

    See Also
    --------
    scipp.compat.from_xarray

    Examples
    --------
    Convert a scipp DataArray to an xarray DataArray:

      >>> import scipp as sc
      >>> sc_da = sc.DataArray(
      ...     data=sc.array(dims=['x', 'y'], values=[[1, 2], [3, 4]], unit='K'),
      ...     coords={'x': sc.array(dims=['x'], values=[0.0, 1.0], unit='m'),
      ...             'y': sc.array(dims=['y'], values=[10, 20])}
      ... )
      >>> xr_da = sc.compat.to_xarray(sc_da)
      >>> xr_da  # doctest: +SKIP
      <xarray.DataArray (x: 2, y: 2)>
      array([[1, 2],
             [3, 4]])
      Coordinates:
        * x        (x) float64 0.0 1.0
        * y        (y) int64 10 20
      Attributes:
          units:    K

    Round-trip conversion preserves data:

      >>> roundtrip = sc.compat.from_xarray(sc.compat.to_xarray(sc_da))  # doctest: +SKIP
      >>> roundtrip  # doctest: +SKIP
      <scipp.DataArray>
      Dimensions: Sizes[x:2, y:2, ]
      Coordinates:
      * x                         float64              [m]  (x)  [0, 1]
      * y                           int64  [dimensionless]  (y)  [10, 20]
      Data:
                                    int64              [K]  (x, y)  [1, 2, 3, 4]
    """  # noqa: E501

    if isinstance(obj, Variable):
        return _to_xarray_variable(obj)
    elif isinstance(obj, DataArray):
        return _to_xarray_dataarray(obj)
    elif isinstance(obj, Dataset):
        return _to_xarray_dataset(obj)
    else:
        raise ValueError(f"to_xarray: cannot convert type '{type(obj)}'")


def _from_xarray_variable(xr_obj: xr.Variable | xr.DataArray) -> Variable:
    """Converts an xarray Coordinate or the data in a DataArray to a scipp.Variable."""
    unit = xr_obj.attrs.get('units', None)
    return Variable(
        dims=xr_obj.dims,
        values=xr_obj.values,
        unit=Unit(unit) if unit is not None else default_unit,
    )


def _to_xarray_variable(var: Variable) -> xr.Variable:
    """Converts a scipp.Variable to a dict containing dims, values and unit for storing
    in either an xarray Coordinate or DataArray.
    """
    import xarray as xr

    if var.is_binned:
        raise ValueError("Xarray does not support binned data.")
    if var.variances is not None:
        warn(
            "Variances of variable were stripped when converting to Xarray.",
            stacklevel=3,
        )
    attrs = {'units': str(var.unit)} if var.unit is not None else None
    return xr.Variable(dims=var.dims, data=var.values, attrs=attrs)


def _from_xarray_dataarray(da: xr.DataArray) -> DataArray:
    """Converts an xarray.DataArray object to a scipp.DataArray object."""
    if da.attrs and set(da.attrs) != {"units"}:
        warn(
            "Input data contains some attributes which have been dropped during the "
            "conversion.",
            stacklevel=3,
        )
    coords = {
        f"{name}": _from_xarray_variable(coord) for name, coord in da.coords.items()
    }
    scipp_da = DataArray(
        data=_from_xarray_variable(da),
        coords=coords,
        name=getattr(da, "name", None) or "",
    )
    for name in da.coords:
        if name not in da.indexes:
            scipp_da.coords.set_aligned(f'{name}', False)
    return scipp_da


def _to_xarray_dataarray(da: DataArray) -> xr.DataArray:
    """Converts a scipp.DataArray object to an xarray.DataArray object."""
    import xarray as xr

    if da.masks:
        warn(
            "Some masks were found in the DataArray. "
            "These have been removed when converting to Xarray.",
            stacklevel=3,
        )
    out = xr.DataArray(_to_xarray_variable(da.data))
    for key, coord in da.coords.items():
        for dim in coord.dims:
            if da.coords.is_edges(key, dim=dim):
                raise ValueError("Xarray does not support coordinates with bin edges.")
        out.coords[key] = _to_xarray_variable(coord)
    out = out.drop_indexes(key for key, coord in da.coords.items() if not coord.aligned)
    return out


def _from_xarray_dataset(ds: xr.Dataset) -> Dataset:
    """Converts an xarray.Dataset object to a scipp.Dataset object."""
    if ds.attrs:
        warn(
            "Input data contains some attributes which have been dropped during the "
            "conversion.",
            stacklevel=3,
        )
    sc_data = {str(k): _from_xarray_dataarray(v) for k, v in ds.items()}
    # The non-indexed coordinates of items also show up as global coordinates in an
    # Xarray dataset, so we make sure we exclude those when we add the remaining coords,
    # after creating the dataset from the individual data arrays.
    coords_in_data_arrays = []
    for item in ds.values():
        coords_in_data_arrays += list(item.coords.keys())
    return Dataset(
        data=sc_data,
        coords={
            str(key): _from_xarray_variable(ds.coords[key])
            for key in (set(ds.coords.keys()) - set(coords_in_data_arrays))
        },
    )


def _to_xarray_dataset(ds: Dataset) -> xr.Dataset:
    """Converts a scipp.Dataset object to an xarray.Dataset object."""
    import xarray as xr

    return xr.Dataset(
        data_vars={k: _to_xarray_variable(v.data) for k, v in ds.items()},
        coords={c: _to_xarray_variable(coord) for c, coord in ds.coords.items()},
    )
