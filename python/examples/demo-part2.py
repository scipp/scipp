#| # Dataset in a Nutshell - Part 2: Multi-dimensional datasets
#|
#| This is the continuation of [Dataset in a Nutshell - Part 1](demo-part1.ipynb).
#|
#| ## Creation, slicing, and visualization

import numpy as np

import dataset as ds
from dataset import Dim, Coord, Data, Attr

#| To create variables with more than one dimension we specify a list of dimension labels and provide data with a corresponding shape. When inserted into a dataset it is important to note that while the dimensions extents have to match, individual variables may have transposed memory layout.

d = ds.Dataset()
d[Coord.X] = ([Dim.X], np.arange(11.0))
d[Coord.Y] = ([Dim.Y], np.arange(11.0))
d[Coord.Z] = ([Dim.Z], np.arange(11.0))
d[Data.Value, "alice"] = ([Dim.Z, Dim.Y, Dim.X], np.random.rand(10, 10, 10))
d[Data.Variance, "alice"] = d[Data.Value, "alice"]
d[Data.Value, "bob"] = ([Dim.X, Dim.Z], np.arange(0.0, 10.0, 0.1).reshape(10, 10))
d[Data.Variance, "bob"] = d[Data.Value, "bob"]

#| Note that in this example the coordinates are exceeding the shape of the data by 1.
#| This means that the coordinates represent bin edges:

d

#| To slice in multiple dimensions, we can simply chain the slicing notation used previously for 1D data.
#| This gives us a number of different options for visualizing our data:

ds.table(d[Dim.X, 5][Dim.Z, 2])

#-------------------------------

ds.plot(d.subset[Data.Value, 'bob'])

#-------------------------------

ds.plot(d.subset[Data.Value, 'alice'])

#-------------------------------

ds.plot(d[Dim.X, 8][Dim.Y, 2])

#| Operations automatically broadcast based on dimension labels. In contrast to `numpy` or `MATLAB` there is no need to keep track of dimension order.

d.subset['alice'] -= d.subset['bob']
d.subset['alice'] -= d.subset['alice'][Dim.Y, 5]
ds.plot(d.subset[Data.Value, 'alice'])

#| ### Exercise 1
#|
#| Remove the surface layer of the volume, i.e., remove the first and last slice in each of the dimensions.

#| ### Solution 1
#| Note the important call to `copy()`. If we omit it, `d` will just be a multi-dimensional slice of the larger volume (which is kept alive), wasting memory and preventing further modification, such as insertion of other variables.

d = d[Dim.X, 1:-1][Dim.Y, 1:-1][Dim.Z, 1:-1].copy()
d

#| ## More advanced operations with multi-dimensional dataset
#| Operations like `concatenate` and `sort` work just like with one-dimensional datasets.
#|
#| ### Exercise 2
#| - Try to concatenate the dataset with itself along the X dimensions. Why does this fail?
#| - Make a copy of the dataset, add an offset to the X coordinate to fix the issue, and try to concatenate again.
#|
#| ### Solution 2
#| With a data extent of, e.g. `8` in this case, bin edges have extent `9`.
#| Naive concatenation would thus lead a new data extent of `16` and a coordinate extent of `18`, which is meaningless and thus prevented.
#| In this `concatenate` merges the last edge of the first input with the first edge of the second input, if compatible.

try:
    d = ds.concatenate(d, d, Dim.X)
except RuntimeError:
    print("Failed as expected!")
offset = d.copy()
offset[Coord.X] += ds.Variable(8.0, unit=ds.units.m)
combined = ds.concatenate(d, offset, Dim.X)
ds.plot(combined.subset[Data.Value, 'alice'])

#| Another available operation is `rebin`.
#| This is only for count-data or count-density-data, so we have to set an appropriate unit first:

new_x = ds.Variable(Coord.X, [Dim.X], d[Coord.X].numpy[::2])
d[Data.Value, 'alice'].unit = ds.units.counts
d[Data.Variance, 'alice'].unit = ds.units.counts *  ds.units.counts
d[Data.Value, 'bob'].unit = ds.units.counts
d[Data.Variance, 'bob'].unit = ds.units.counts *  ds.units.counts
d = ds.rebin(d, new_x)
d

#| ## Interaction with `numpy`
#| Variable in a dataset are exposed in a `numpy`-compatible buffer format, so we can directly hand them to `numpy` functions.

d[Data.Value, 'alice'] = np.sin(d[Data.Value, 'alice'])

#| Direct access to the `numpy`-like underlying data array is possible using the `numpy` property. This is now a multi-dimensional array:

d[Data.Value, 'alice'].numpy

#| ### Exercise 3
#| 1. Use `ds.mean` to compute the mean of the data for Alice along the Z dimension.
#| 2. Do the same with `numpy`, what are the complications you encounter, that are not present when using the dataset?
#|
#| ### Solution 3

help(ds.mean)

#-------------------------------

mean = ds.mean(d.subset['alice'], Dim.Z)

#| When using `numpy` to compute the mean:
#| - We must remember (or lookup) which dimension corresponds to the Z dimensions.
#| - We need a separate call for values and variances.
#| - We need to manually scale the variance with the inverse square of the number of data points to get the variance of the mean.

np_value = np.mean(d[Data.Value, 'alice'], axis=0)
np_variance = np.mean(d[Data.Variance, 'alice'], axis=0)
np_variance /= np.sqrt(d.dimensions[Dim.Z])

