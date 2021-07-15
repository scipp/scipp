from __future__ import annotations

from typing import Union, TYPE_CHECKING, Dict

from .. import Dataset, DataArray, Variable, scalar, VariableLike

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


def _to_scipp_coords(xr_obj: Union[xr.Dataset, xr.DataArray],
                     index: bool) -> Dict[str, VariableLike]:
    """
    Converts xarray coords to scipp coords.

    Note: only converts dimension coords

    :param xr_obj: The xarray object to convert coordinates of
    :param index: Whether to return index coordinates (True) or
        non-index coordinates (False)
    :return: a dictionary suitable for passing to sc.DataArray or sc.Dataset
    """
    coords = {}

    for coord_name, coord in xr_obj.coords.items():
        is_index = (coord_name in xr_obj.indexes)
        if is_index == index:
            coords[coord_name] = Variable(dims=coord.dims, values=coord.values)
    return coords


def from_xarray_dataarray(da: xr.DataArray) -> DataArray:
    """
    Converts an xarray.DataArray object to a scipp.DataArray object.

    :param da: an xarray.DataArray object to be converted.
    :return: the converted scipp DataArray object.
    """
    sc_attrs = {attr: scalar(da.attrs[attr]) for attr in da.attrs}

    for name, dim in _to_scipp_coords(da, index=False).items():
        sc_attrs[f"coord_{name}"] = dim

    return DataArray(data=Variable(values=da.values, dims=da.dims),
                     coords=_to_scipp_coords(da, index=True),
                     attrs=sc_attrs,
                     name=da.name or "")


def from_xarray_dataset(ds: xr.Dataset) -> Dataset:
    """
    Converts an xarray.Dataset object to a scipp.Dataset object.

    :param ds: an xarray.Dataset object to be converted.
    :return: the converted scipp dataset object.
    """
    sc_data = {k: from_xarray(v) for k, v in ds.data_vars.items()}

    return Dataset(data=sc_data)
