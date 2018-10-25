from dataset import *
import numpy as np
import xarray as xr
import matplotlib.pyplot as plt

lx = 20
ly = 30
lz = 40
d = Dataset()
dims = Dimensions()
dims.add(Dim.X, lx)
dims.add(Dim.Y, ly)
dims.add(Dim.Z, lz)

dimsX = Dimensions()
dimsY = Dimensions()
dimsZ = Dimensions()
dimsX.add(Dim.X, lx)
dimsY.add(Dim.Y, 2*ly)
dimsZ.add(Dim.Z, lz)

d.insert(Coord.X, dimsX, np.arange(lx))
d.insert(Coord.Z, dimsZ, np.arange(lz))
d.insert(Data.Value, "name", dims, np.arange(lx*ly*lz))

d = concatenate(Dim.Y, d, d)
d.insert(Coord.Y, dimsY, np.arange(2*ly))

def Dataset_iter(self):
    yield self[Coord.X]
    yield self[Coord.Y]
    yield self[Coord.Z]
    yield self[Data.Value, 'name']

setattr(Dataset, "__iter__", Dataset_iter)

dim_name = {Dim.X:'X', Dim.Y:'Y', Dim.Z:'Z'}

def as_xarray(dataset):
    ds = xr.Dataset()
    data = dict()
    coords = dict()
    def xarray_name(var):
        names = {0:'X', 1:'Y', 2:'Z', 34:'Value'}
        if var.is_coord:
            return names[var.type]
        else:
            return names[var.type] + ':' + var.name
    for var in dataset:
        print(var.dimensions.labels)
        if var.is_coord:
            ds.coords[xarray_name(var)] = ([dim_name[dim] for dim in var.dimensions.labels], var.numpy)
        else:
            ds[xarray_name(var)] = ([dim_name[dim] for dim in var.dimensions.labels], var.numpy)
    return ds


ds = as_xarray(d)
ds *= 1.5

print(ds['Value:name'][10])
ds['Value:name'][10, 5:10,5:10].plot()
plt.savefig('test.png')

# Key differences to xarray:
# - support bin edges
# - support units
# - support propagation of uncertainties
# - C++
# - support Mantid-specific data (Run, instrument mappings, ...)
