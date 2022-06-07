# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)

from __future__ import annotations

from typing import Union, TYPE_CHECKING
from warnings import warn

from ..core import Dataset, DataArray, Unit, Variable
from ..core import scalar
from ..typing import VariableLike

if TYPE_CHECKING:
    import xarray as xr


def from_xarray(obj: Union[xr.DataArray, xr.Dataset]) -> VariableLike:
    """Convert an xarray DataArray or Dataset to the corresponding scipp object.

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

    if isinstance(obj, xr.DataArray):
        return _from_xarray_dataarray(obj)
    elif isinstance(obj, xr.Dataset):
        return _from_xarray_dataset(obj)
    else:
        raise ValueError(f"from_xarray: cannot convert type '{type(obj)}'")


def to_xarray(obj: VariableLike) -> Union[xr.DataArray, xr.Dataset]:
    """Convert a scipp DataArray or Dataset to the corresponding xarray object.

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

    if isinstance(obj, DataArray):
        return _to_xarray_dataarray(obj)
    elif isinstance(obj, Dataset):
        return _to_xarray_dataset(obj)
    else:
        raise ValueError(f"to_xarray: cannot convert type '{type(obj)}'")


def _var_from_xarray(xr_obj: Union[xr.Coordinate, xr.DataArray]) -> Variable:
    unit = xr_obj.attrs.get('units', '')
    return Variable(dims=xr_obj.dims, values=xr_obj.values, unit=Unit(unit))


def _var_to_xarray(var: Variable) -> dict:
    if var.bins is not None:
        raise ValueError("Xarray does not support binned data.")
    out = {'dims': var.dims, 'values': var.values}
    if var.unit is not None:
        out['unit'] = str(var.unit)
    if var.variances is not None:
        warn("Variances of data were stripped when converting to Xarray.")
    return out


def _from_xarray_dataarray(da: xr.DataArray) -> DataArray:
    """Converts an xarray.DataArray object to a scipp.DataArray object.
    """
    coords = {}
    attrs = {attr: scalar(da.attrs[attr]) for attr in da.attrs if attr != "units"}

    for name, coord in da.coords.items():
        if name in da.indexes:
            coords[name] = _var_from_xarray(coord)
        else:
            attrs[f"{name}"] = _var_from_xarray(coord)

    return DataArray(data=_var_from_xarray(da),
                     coords=coords,
                     attrs=attrs,
                     name=da.name or "")


def _to_xarray_dataarray(da: DataArray) -> xr.DataArray:
    """Converts a scipp.DataArray object to an xarray.DataArray object.
    """
    import xarray as xr
    data = _var_to_xarray(da.data)
    coords = {}
    for key, coord in da.coords.items():
        if da.coords.is_edges(key):
            raise ValueError("Xarray does not support coordinates with bin edges.")
        coords[key] = _var_to_xarray(coord)

    out = xr.DataArray(data=data['values'],
                       dims=data['dims'],
                       coords={key: coord['values']
                               for key, coord in coords.items()})
    if 'unit' in data:
        out.attrs['units'] = data['unit']

    for key, coord in coords.items():
        if 'unit' in coord:
            out.coords[key].attrs['units'] = coord['unit']

    return out


def _from_xarray_dataset(ds: xr.Dataset) -> Dataset:
    """Converts an xarray.Dataset object to a scipp.Dataset object.
    """
    sc_data = {k: _from_xarray_dataarray(v) for k, v in ds.items()}
    return Dataset(data=sc_data)


def _to_xarray_dataset(ds: Dataset) -> xr.Dataset:
    """Converts a scipp.Dataset object to an xarray.Dataset object.
    """
    import xarray as xr
    return xr.Dataset({k: to_xarray_dataarray(v) for k, v in ds.items()})
