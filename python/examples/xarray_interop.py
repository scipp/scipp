import holoplot
import scippy as sp
import numpy as np
import xarray as xr
import matplotlib.pyplot as plt

lx = 200
ly = 300
lz = 400
d = sp.Dataset()
dims = sp.Dimensions()
dims.add(sp.Dim.X, lx)
dims.add(sp.Dim.Y, ly)
dims.add(sp.Dim.Z, lz)

dimsX = sp.Dimensions()
dimsY = sp.Dimensions()
dimsZ = sp.Dimensions()
dimsX.add(sp.Dim.X, lx)
dimsY.add(sp.Dim.Y, 2 * ly)
dimsZ.add(sp.Dim.Z, lz)

d.insert(sp.Coord.X, dimsX, np.arange(lx))
d.insert(sp.Coord.Z, dimsZ, np.arange(lz))
d.insert(sp.Data.Value, "name", dims, np.random.random(lx * ly * lz))

d = sp.concatenate(sp.Dim.Y, d, d)
d.insert(sp.Coord.Y, dimsY, np.arange(2 * ly))


def Dataset_iter(self):
    yield self[sp.Coord.X]
    yield self[sp.Coord.Y]
    yield self[sp.Coord.Z]
    yield self[sp.Data.Value, 'name']


setattr(sp.Dataset, "__iter__", sp.Dataset_iter)

dim_name = {sp.Dim.X: 'X', sp.Dim.Y: 'Y', sp.Dim.Z: 'Z'}


def as_xarray(dataset):
    ds = xr.Dataset()

    def xarray_name(var):
        names = {0: 'X', 1: 'Y', 2: 'Z', 34: 'Value'}
        if var.is_coord:
            return names[var.type]
        else:
            return names[var.type] + ':' + var.name
    for var in dataset:
        print(var.dimensions.labels)
        if var.is_coord:
            ds.coords[xarray_name(var)] = \
                ([dim_name[dim] for dim in var.dimensions.labels], var.numpy)
        else:
            ds[xarray_name(var)] = \
                ([dim_name[dim] for dim in var.dimensions.labels], var.numpy)
    return ds


ds = as_xarray(d)
ds *= 1.5

print(ds['Value:name'][10])
ds['Value:name'][10, 5:10, 5:10].plot()
plt.savefig('test.png')

# pip3 install hvplot holoplot
sliceview = ds['Value:name'].hvplot.image(groupby='Z', height=1000, width=1000)
holoplot.show(sliceview)

# Key differences to xarray:
# - support bin edges
# - support units
# - support propagation of uncertainties
# - C++
# - support Mantid-specific data (Run, instrument mappings, ...)
