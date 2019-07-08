#| # Dataset in a Nutshell
#| This demonstrates key functionality and usage of the `dataset` library.
#|
#| ## Getting started
#| ### What is a `Dataset`?
#| There are two basic analogies to aid in thinking about a `Dataset`:
#| 1. As a `dict` of `numpy.ndarray`s, with the addition of named dimensions and units.
#| 2. As a table.
#|
#| ### Creating a dataset

import numpy as np

import dataset as ds
from dataset import Dim, Coord, Data, Attr

#-------------------------------

d = ds.Dataset()
d

#| ## Using `Dataset` as a table
#| We can not only think about a dataset as a table, we can also use it as one.
#| This will demonstrate the basic ways of creating datasets and interacting with them.

d[Coord.Row] = ([Dim.Row], np.arange(3))
d[Data.Value, "alice"] = ([Dim.Row], [1.0,1.1,1.2])
d[Data.Variance, "alice"] = ([Dim.Row], [0.01,0.01,0.02])
d

#| The datatype (`dtype`) is derived from the provided data, so passing `np.arange(3)` will yield a variable (column) containing 64-bit integers.
#|
#| Datasets with up to one dimension can be displayed as a simple table:

ds.table(d)

#| A variable (column) in a dataset (table) is identified by its tag (`Coord.Row`, `Data.Value`, or `Data.Variance`) and a name (`"alice"`). The name is "grouping" variables of different tags that are semantically connected. The main use for this is to link uncertainties (variances) to their data.
#|
#| Each variable (column) comes with a physical unit attached to it, which we should set up correctly as early as possible.

d[Data.Value, "alice"].unit = ds.units.m
d[Data.Variance, "alice"].unit = ds.units.m * ds.units.m
ds.table(d)

#| Operations between columns are supported by indexing into a dataset with a tag (and name). A column group can also be selected and operated on, using the `subset` property of `Dataset`.

d[Data.Value, "bob"] = d[Data.Value, "alice"]
d

#-------------------------------

d[Data.Variance, "bob"] = d[Data.Variance, "alice"]
d.subset["bob"] += d.subset["alice"]
ds.table(d)

#-------------------------------

ds.plot(d)

#| Operations between rows are supported by indexing into a dataset with a dimension label and an index.

d[Dim.Row, 1] += d[Dim.Row, 2]
ds.table(d)

#| Note the key advantage over `numpy` or `MATLAB`: We specify the index dimension, so we always know which dimension we are slicing. The advantage is not so apparent in 1D, but will become clear once we move to higher-dimensional data.

#| Units and uncertainties are handled automatically in operations.

d *= d
ds.table(d)

#| ### Summary
#| There is a number of ways to select and operate on a single row, a range of rows, a single variable (column) or multiple variables (columns) of a dataset:

# Single row (dropping corresponding coordinates)
ds.table(d[Dim.Row, 0])
# Size-1 row range (keeping corresponding coordinates)
ds.table(d[Dim.Row, 0:1])
# Range of rows
ds.table(d[Dim.Row, 1:3])
# Single variable
ds.table(d[Data.Value, 'alice'])
# Subset of variables with given name, keeping coordinates
ds.table(d.subset['alice'])
# Subset containing a single (data) variable, in addition to coordinates
ds.table(d.subset[Data.Value, 'alice'])

#| ### Exercise 1
#|
#| Using the slicing notation, create a new table (or replace the existing dataset `d`) by one that does not contain the first and last row of `d`.


#| ### Solution 1
#|
#| Scroll down for solution.
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V

table = d[Dim.Row, 1:-1].copy()

# Or:
# from copy import copy
# table = copy(d[Dim.Row, 1:-1])
table

#| ### Exercise 2
#| 1. Combining row slicing and "column" slicing, add the last row of the data for Alice to the first row of data for Bob.
#| 2. Using the slice-range notation `a:b`, try adding the last two rows to the first two rows. Why does this fail?


#| ### Solution 2
#|
#| Scroll down for solution.
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V

d.subset["bob"][Dim.Row, 0] += d.subset["alice"][Dim.Row, -1]
ds.table(d)

#| If a range is given when slicing, the corresponding coordinate is preserved, and operations between misaligned data is prevented.

try:
    d.subset["bob"][Dim.Row, 0:2] += d.subset["alice"][Dim.Row, 1:3]
except RuntimeError:
    print("Failed as expected!")

#| We can operate with individual variables to circumvent the safety catch:

d[Data.Value, "bob"][Dim.Row, 0:2] += d[Data.Value, "alice"][Dim.Row, 1:3]

#| We can also imagine ways to explicitly drop coordinates from a subset, e.g., `d.subset['bob'].drop_coords()`, to allow for direct operation with subset. This is currently not supported.

#| ## More advanced operations with tables
#| In addition to binary operators, basic functions like `concatenate`, `sort`, and `merge` are available.

d = ds.concatenate(d[Dim.Row, 0:3], d[Dim.Row, 1:3], Dim.Row)
d = ds.sort(d, Coord.Row)
eve = ds.Dataset()
eve[Data.Value, 'eve'] = ([Dim.Row], np.arange(5).astype(np.float64))
d.merge(eve)
ds.table(d)

#| ### Exercise 3
#| Add the sum of Alice's and Bob's height as a new variable (column) to the dataset.


#| ### Solution 3
#|
#| Scroll down for solution.
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V

d.subset['sum'] = d.subset['alice'] + d.subset['bob']
ds.table(d)

#| ### Interaction with `numpy` and scalars
#| Variable in a dataset are exposed in a `numpy`-compatible buffer format, so we can directly hand them to `numpy` functions.

d[Data.Value, 'eve'] = np.exp(d[Data.Value, 'eve'])
ds.table(d)

#| Direct access to the `numpy`-like underlying data array is possible using the `numpy` property:

d[Data.Value, 'eve'].numpy

#| ### Exercise 4
#| 1. As above for `np.exp` applied to the data for Eve, apply a `numpy` function to the data for Alice.
#| 2. What happens to the unit and uncertanties when modifying data with external code such as `numpy`?


#| ### Solution 4
#|
#| Scroll down for solution.
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V

d[Data.Value, 'alice'] = np.sin(d[Data.Value, 'alice'])
ds.table(d)

#| Numpy operations are not aware of the unit and uncertainties. Therefore the result is garbage, unless the user has ensured herself that units and uncertainties are handled manually.
#|
#| Corollary: Whenever available, built-in operators and functions should be preferred over the use of `numpy`.

#| ### Exercise 5
#| 1. Try adding a scalar value such as `1.5` to the data for Eve.
#| 2. Try the same for Alice or Bob. Why is it not working?


#| ### Solution 5
#|
#| Scroll down for solution.
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V
#|
#| V

d.subset['eve'] += 1.5
ds.table(d)

#| The data for Alice has a unit, so a direct addition with a dimensionless quantity fails:

try:
    d.subset['alice'] += 1.5
except RuntimeError:
    print("Failed as expected!")

#| We can use `Variable` to provide scalar quantity with attached unit:

d.subset['alice'] += ds.Variable(1.5, unit=ds.units.m*ds.units.m)
ds.table(d)

