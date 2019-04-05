# Sparse dimension support

In our case, the main application of sparse dimensions is neutron event data.
So far we have assumed that a dataset is not aware of the dimension of the event data, since it has a different length at every point.
For example, event data could be stored as a nested dataset for each of the points in the volume of the outer dataset (see https://github.com/scipp/scipp/blob/master/scippy/demo/demo-part3.ipynb for an example).
This works, but it has the drawback of making handling such data different and more cumbersome, since we must access nested structures and provide special routines.

Here we discuss another option, supporting sparse data by making `Dataset` and `Variable` compatible with sparse dimensions.
This could work as follows:

1. There can be only a single sparse dimension per `Variable` or `Dataset`, and it must be the innermost dimension.

2. `class Dimensions` would not store the extent of the sparse dimension, but an invalid extent (e.g., -1). For display, we simply print "sparse", not "-1".

3. Storing event data:
   - `Coord.Tof`, `Coord.Energy`, ... used for, e.g., time-of-flight, .... This removes the need for having a number of special tags for events, such as `Data.EventTof`, `Data.EventEnergy`, ..., which are required in the current model without sparse-dimension support.
   - `Coord.PulseTime` for pulse times.
   - `Data.Value` and `Data.Variance` for weights. These can be omitted for unweighted events.
   - Coordinates for event data would need to be named, since they are specific for their data --- data for, e.g., sample and vanadium runs would *not* share these coordinates.
     This furthermore implies that we must allow for mismatches in the sparse dimension, i.e., a single dataset can hold two variables with sparse dimension that have mismatching sparse extents.
   - Does this imply that we cannot easily store event- and histogram-data in the same dataset (with the same name)?
     Can we have some variables where, e.g., `Dim.Tof` is dense, whereas there are others where it is sparse? The simplest example would be storing and (unnamed) `Coord.Tof` in a dataset together with events, which could later be binned to that coordinate, similar to Mantid's `EventWorkspace`.
     - Could support sparse dimension mismatching dense dimension extent.

4. Access.
   - In general, `Variable::span<T>` can (and usually should) not work for sparse data.
     We do not want to expose the sparse storage layout on the interface, since this would prevent future changes.
     In particular, we should avoid being forced to store sparse data as a single contiguous block.
   - Use something like `Variable::sparseProxy<T>`, returning `SparseProxy<T>`, which has `SparseProxy<T>::span(const scipp::index i)`, with `i` in range `[0,...,volume_of_non_sparse_dimensions]`. The latter returns a span to the range of items of type `T` (the 1D list of values for a particular event list). That is, we do commit to a contiguous layout of data within a single "event list".

5. Slicing.
   - We cannot slice in the sparse dimension.
   - Investigate whether a fully sliced variable can appear as non-sparse, i.e., when all but the sparse dimension have been removed.
   - Special value-based slicing methods could be supported, but would obviously be less efficient than slicing dense dimensions.

6. Operations.
   - `sort`, `concatenate`, `filter`, ... work well and are useful for sparse dimensions.
   - binary operations do not work, need to provide alternatives.
     - use `concatenate` instead of `+`.
     - instead of `-`, add weights (negative for second operand) and then concatenate.
     - ...

7. Data layout. With the access described in 4.), we can choose to store sparse data in various ways.
   - One option is to use a `Variable` with element type `std::vector<T>` or `boost::small_vector<T>`, but an extra negative dimension as described in 2.).
     This could potentially be done with only minor changes to the related classes like `VariableConcept`.
     We do however need to add support for binary operations, but this is something that would also have been required if event data is stored in the current way without sparse-dimension support.
   - Another option is to use a linearized layout. This is less efficient if appending/concatenating in the sparse dimension is required, but could be faster and more efficient in other cases. However, it would require implementing a new `DataModel`, since we also need to store an index in addition to the data.
