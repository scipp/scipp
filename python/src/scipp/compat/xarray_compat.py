import typing

import xarray
import scipp


def from_xarray(xarray_obj: typing.Union[xarray.DataArray, xarray.Dataset]):
    if isinstance(xarray_obj, xarray.DataArray):
        return from_xarray_dataarray(xarray_obj)
    elif isinstance(xarray_obj, xarray.Dataset):
        return from_xarray_dataset(xarray_obj)
    else:
        raise ValueError(
            f"from_xarray: cannot convert type '{type(xarray_obj)}'")


def from_xarray_dataarray(
        xarray_dataarray: xarray.DataArray) -> scipp.DataArray:
    sc_coords = {}
    sc_attribs = {}

    for coord in xarray_dataarray.coords:
        sc_coords[coord] = scipp.Variable(
            dims=[coord],
            values=xarray_dataarray.coords[coord].values,
        )

    for attrib in xarray_dataarray.attrs:
        sc_attribs[attrib] = scipp.scalar(xarray_dataarray.attrs[attrib])

    return scipp.DataArray(data=scipp.Variable(values=xarray_dataarray.values,
                                               dims=xarray_dataarray.dims),
                           coords=sc_coords,
                           attrs=sc_attribs,
                           name=xarray_dataarray.name or "")


def from_xarray_dataset(xarray_dataset: xarray.Dataset,
                        *,
                        attrib_prefix="attrib_") -> scipp.Dataset:
    sc_data = {}
    sc_coords = {}

    for data_key in xarray_dataset.data_vars:
        sc_data[data_key] = from_xarray_dataarray(
            xarray_dataset.data_vars[data_key])

    for coord in xarray_dataset.coords:
        sc_coords[coord] = scipp.Variable(
            dims=[coord],
            values=xarray_dataset.coords[coord].values,
        )

    for attrib in xarray_dataset.attrs:
        attrib_name = "{}{}".format(attrib_prefix, attrib)
        if attrib_name in sc_data:
            raise ValueError(
                f"Attribute {attrib} would collide with an existing "
                f"data object '{attrib_name}' (using attrib_prefix "
                f"'{attrib_prefix}'). Specify a different attrib_prefix "
                f"in the call to from_xarray_dataset")

        sc_data[attrib_name] = scipp.scalar(xarray_dataset.attrs[attrib])

    return scipp.Dataset(
        data=sc_data,
        coords=sc_coords,
    )
