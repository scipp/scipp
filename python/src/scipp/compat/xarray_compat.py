import xarray
import scipp


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
        sc_attribs[attrib] = scipp.Variable(
            dims=[], values=xarray_dataarray.attrs[attrib])

    return scipp.DataArray(data=scipp.Variable(values=xarray_dataarray.values,
                                               dims=xarray_dataarray.dims),
                           coords=sc_coords,
                           attrs=sc_attribs,
                           name=xarray_dataarray.name or "")


def from_xarray_dataset(xarray_dataset: xarray.Dataset) -> scipp.Dataset:
    return scipp.Dataset(
        coords=xarray_dataset.coords,
        attrs=xarray_dataset.attrs,
    )
