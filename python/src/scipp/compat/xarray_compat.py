from __future__ import annotations

from typing import Union, TYPE_CHECKING

from .. import Dataset, DataArray, Variable, scalar

if TYPE_CHECKING:
    import xarray as xr


def from_xarray(obj: Union[xr.DataArray, xr.Dataset]):
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


def from_xarray_dataarray(da: xr.DataArray) -> DataArray:
    """
    Converts an xarray.DataArray object to a scipp.DataArray object.

    :param da: an xarray.DataArray object to be converted.
    :return: the converted scipp DataArray object.
    """
    sc_coords = {}
    sc_attribs = {}

    for coord in da.coords:
        sc_coords[coord] = Variable(
            dims=[coord],
            values=da.coords[coord].values,
        )

    for attrib in da.attrs:
        sc_attribs[attrib] = scalar(da.attrs[attrib])

    return DataArray(data=Variable(values=da.values, dims=da.dims),
                     coords=sc_coords,
                     attrs=sc_attribs,
                     name=da.name or "")


def from_xarray_dataset(ds: xr.Dataset) -> Dataset:
    """
    Converts an xarray.Dataset object to a scipp.Dataset object.

    :param ds: an xarray.Dataset object to be converted.
    :return: the converted scipp dataset object.
    """
    sc_coords = {}

    for coord in ds.coords:
        sc_coords[coord] = Variable(
            dims=[coord],
            values=ds.coords[coord].values,
        )

    return Dataset(
        data={key: from_xarray(var)
              for key, var in ds.data_vars.items()},
        coords=sc_coords,
    )
