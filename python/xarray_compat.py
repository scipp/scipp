import xarray as xr
from . import dataset as ds

def as_xarray(dataset):
    dim_name = {ds.Dim.X:'X', ds.Dim.Y:'Y', ds.Dim.Z:'Z', ds.Dim.Row:'Row'}

    d = xr.Dataset()
    data = dict()
    coords = dict()
    def xarray_name(var):
        names = {ds.Coord.X:'X', ds.Coord.Y:'Y', ds.Coord.Z:'Z', ds.Coord.RowLabel:'RowLabel', ds.Data.Value:'Value'}
        if var.is_coord:
            # var.tag returns the `Tag` base class, so dict lookup does not work. How can we fix this?
            for key in names:
                if var.tag == key:
                    return names[key]
        else:
            for key in names:
                if var.tag == key:
                    return names[key] + ':' + var.name
    for var in dataset:
        dims = var.dimensions
        labels = dims.labels
        if not var.is_coord:
            try:
                d[xarray_name(var)] = ([dim_name[dim] for dim in labels], var.numpy)
            except:
                d[xarray_name(var)] = ([dim_name[dim] for dim in labels], var.data)
    for var in dataset:
        dims = var.dimensions
        labels = dims.labels
        if var.is_coord:
            edges = False
            for label in labels:
                if dims.size(label) != d.dims[dim_name[label]]:
                    edges = True
            if edges:
                d.coords[xarray_name(var)] = ([dim_name[dim] for dim in labels], 0.5*(var.numpy[:-1] + var.numpy[1:]))
            else:
                try:
                    d.coords[xarray_name(var)] = ([dim_name[dim] for dim in labels], var.numpy)
                except:
                    d.coords[xarray_name(var)] = ([dim_name[dim] for dim in labels], var.data)
    return d
