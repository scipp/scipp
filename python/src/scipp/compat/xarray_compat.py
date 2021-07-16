from __future__ import annotations

from typing import Union, TYPE_CHECKING

from .. import Dataset, DataArray, Variable, scalar, VariableLike, Unit

if TYPE_CHECKING:
    import xarray as xr


def from_xarray(obj: Union[xr.DataArray, xr.Dataset]) -> VariableLike:
    """
    Convenience method to convert an xarray object into the corresponding
    scipp object.

    If you know in advance what type of object you need to convert, you can
    also call from_xarray_dataarray or from_xarray_dataset directly.

    :param obj: the xarray object to convert; must be either an xarray
        DataArray or Dataset object.
    :return: the converted scipp object.
    """
    import xarray as xr

    if isinstance(obj, xr.DataArray):
        return from_xarray_dataarray(obj)
    elif isinstance(obj, xr.Dataset):
        return from_xarray_dataset(obj)
    else:
        raise ValueError(f"from_xarray: cannot convert type '{type(obj)}'")


def _var_from_xarray(xr_obj: Union[xr.Coordinate, xr.DataArray]):
    unit = xr_obj.attrs.get('units', '')
    return Variable(dims=xr_obj.dims, values=xr_obj.values, unit=Unit(unit))


def from_xarray_dataarray(da: xr.DataArray) -> DataArray:
    """
    Converts an xarray.DataArray object to a scipp.DataArray object.

    :param da: an xarray.DataArray object to be converted.
    :return: the converted scipp DataArray object.
    """
    coords = {}
    attrs = {
        attr: scalar(da.attrs[attr])
        for attr in da.attrs if attr != "units"
    }

    for name, coord in da.coords.items():
        if name in da.indexes:
            coords[name] = _var_from_xarray(coord)
        else:
            attrs[f"coord_{name}"] = _var_from_xarray(coord)

    return DataArray(data=_var_from_xarray(da),
                     coords=coords,
                     attrs=attrs,
                     name=da.name or "")


def from_xarray_dataset(ds: xr.Dataset) -> Dataset:
    """
    Converts an xarray.Dataset object to a scipp.Dataset object.

    :param ds: an xarray.Dataset object to be converted.
    :return: the converted scipp dataset object.
    """
    sc_data = {k: from_xarray(v) for k, v in ds.items()}
    return Dataset(data=sc_data)
