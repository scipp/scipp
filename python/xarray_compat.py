import xarray as xr
from . import dataset as ds

def as_xarray(dataset):
    dim_name = {ds.Dim.X:'X', ds.Dim.Y:'Y', ds.Dim.Z:'Z', ds.Dim.Row:'Row'}

    d = xr.Dataset()
    data = dict()
    coords = dict()
    def xarray_name(var):
        names = {0:'X', 1:'Y', 2:'Z', 13:'RowLabel', 36:'Value'}
        if var.is_coord:
            return names[var.type]
        else:
            return names[var.type] + ':' + var.name
    for var in dataset:
        dims = var.dimensions
        labels = dims.labels
        if var.is_coord:
            try:
                d.coords[xarray_name(var)] = ([dim_name[dim] for dim in labels], var.numpy)
            except:
                d.coords[xarray_name(var)] = ([dim_name[dim] for dim in labels], var.data)
        else:
            try:
                d[xarray_name(var)] = ([dim_name[dim] for dim in labels], var.numpy)
            except:
                d[xarray_name(var)] = ([dim_name[dim] for dim in labels], var.data)
    return d
