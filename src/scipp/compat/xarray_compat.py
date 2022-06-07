# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import Union, TYPE_CHECKING
from warnings import warn

from ..core import Dataset, DataArray, Unit, Variable, scalar, merge
from ..typing import VariableLike
from ..units import default_unit

if TYPE_CHECKING:
    import xarray as xr


def from_xarray(obj: Union[xr.Variable, xr.DataArray, xr.Dataset]) -> VariableLike:
    """Convert an xarray object to the corresponding scipp object.

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


def to_xarray(obj: VariableLike) -> Union[xr.Variable, xr.DataArray, xr.Dataset]:
    """Convert a scipp object to the corresponding xarray object.

    Warning
    -------
    Any masks and variances in the input will be stripped during the conversion.

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
    """

    if isinstance(obj, Variable):
        return _to_xarray_variable(obj)
    elif isinstance(obj, DataArray):
        return _to_xarray_dataarray(obj)
    elif isinstance(obj, Dataset):
        return _to_xarray_dataset(obj)
    else:
        raise ValueError(f"to_xarray: cannot convert type '{type(obj)}'")


def _from_xarray_variable(xr_obj: Union[xr.Coordinate, xr.DataArray]) -> Variable:
    """Converts an xarray Coordinate or the data in a DataArray to a scipp.Variable.
    """
    unit = xr_obj.attrs.get('units', None)
    return Variable(dims=xr_obj.dims,
                    values=xr_obj.values,
                    unit=Unit(unit) if unit is not None else default_unit)


def _to_xarray_variable(var: Variable) -> xr.Variable:
    """Converts a scipp.Variable to a dict containing dims, values and unit for storing
    in either an xarray Coordinate or DataArray.
    """
    import xarray as xr
    if var.bins is not None:
        raise ValueError("Xarray does not support binned data.")
    if var.variances is not None:
        warn("Variances of variable were stripped when converting to Xarray.")
    attrs = {'units': str(var.unit)} if var.unit is not None else None
    return xr.Variable(dims=var.dims, data=var.values, attrs=attrs)


def _from_xarray_dataarray(da: xr.DataArray) -> DataArray:
    """Converts an xarray.DataArray object to a scipp.DataArray object.
    """
    coords = {}
    attrs = {attr: scalar(da.attrs[attr]) for attr in da.attrs if attr != "units"}

    for name, coord in da.coords.items():
        if name in da.indexes:
            coords[name] = _from_xarray_variable(coord)
        else:
            attrs[f"{name}"] = _from_xarray_variable(coord)

    return DataArray(data=_from_xarray_variable(da),
                     coords=coords,
                     attrs=attrs,
                     name=da.name or "")


def _to_xarray_dataarray(da: DataArray) -> xr.DataArray:
    """Converts a scipp.DataArray object to an xarray.DataArray object.
    """
    import xarray as xr
    if da.masks:
        warn("Some masks were found in the DataArray. "
             "These have been removed when converting to Xarray.")
    data = _to_xarray_variable(da.data)
    coords = {}
    for key, coord in {**da.coords, **da.attrs}.items():
        for dim in coord.dims:
            if da.meta.is_edges(key, dim=dim):
                raise ValueError("Xarray does not support coordinates with bin edges.")
        coords[key] = _to_xarray_variable(coord)

    return xr.DataArray(data, coords={key: coord for key, coord in coords.items()})


def _from_xarray_dataset(ds: xr.Dataset) -> Dataset:
    """Converts an xarray.Dataset object to a scipp.Dataset object.
    """
    sc_data = {k: _from_xarray_dataarray(v) for k, v in ds.items()}
    coords_in_data_arrays = []
    for item in ds.values():
        coords_in_data_arrays += list(item.coords.keys())
    return Dataset(data=sc_data,
                   coords={
                       key: _from_xarray_variable(ds.coords[key])
                       for key in (set(ds.coords.keys()) - set(coords_in_data_arrays))
                   })


def _to_xarray_dataset(ds: Dataset) -> xr.Dataset:
    """Converts a scipp.Dataset object to an xarray.Dataset object.
    """
    import xarray as xr
    out = xr.Dataset({k: _to_xarray_dataarray(v) for k, v in ds.items()})
    for key in set(ds.coords.keys()) - set(out.coords.keys()):
        out.coords[key] = _to_xarray_variable(ds.coords[key])
    return out
