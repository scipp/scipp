import xarray as xr
from . import dataset as ds

def name(x):
    return x.__repr__().split('.')[1]

def as_xarray(dataset):
    d = xr.Dataset()
    data = dict()
    coords = dict()
    def xarray_name(var):
        if len(var.name) == 0:
            return name(var.tag)
        else:
            return name(var.tag) + ':' + var.name
    for var in dataset:
        dims = var.dimensions
        labels = dims.labels
        if not var.is_coord:
            try:
                d[xarray_name(var)] = ([name(dim) for dim in labels], var.numpy)
            except:
                d[xarray_name(var)] = ([name(dim) for dim in labels], var.data)
    for var in dataset:
        dims = var.dimensions
        labels = dims.labels
        if var.is_coord:
            edges = False
            for label in labels:
                if dims.size(label) != d.dims[name(label)]:
                    edges = True
            if edges:
                d.coords[xarray_name(var)] = ([name(dim) for dim in labels], 0.5*(var.numpy[:-1] + var.numpy[1:]))
            else:
                try:
                    d.coords[xarray_name(var)] = ([name(dim) for dim in labels], var.numpy)
                except:
                    try:
                        d.coords[xarray_name(var)] = ([name(dim) for dim in labels], var.data)
                    except:
                        print("Error converting {} to xarray, dropping.".format(var))
    return d
