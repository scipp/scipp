# Type-erased dataset prototype

## FAQ

1. Why not workspaces with virtual interface to provide common functionality?
   - Common virtual interfaces are already running into trouble with the current workspaces.
     With more and more generic workspace types we would not be getting anywhere.

1. Why not with static typing of columns, i.e., similar to `std::tuple<Ts...>`?
   - Yields an unreasonably large number of workspace types that probably cannot be handled efficiently in terms of compile times, code size, and complete Python exports.
   - Does not cover cases where we require flexibility at runtime, such as for generic tables.

1. Why not units in type?
   - This is not 100% clear yet, but probably this also yields a unmanageably large number of column types?

1. Why not a nested structure of table-like objects?
   - Apart from an easier implementation of methods for selecting certain slices from a `Dataset` this does not seem to provide advantages and only increases complexity.

1. Why not histograms as the smallest element in a `Dataset`?
   - Cannot handle sharing of the x-axis (bin edges) in an easy way.
     There current approach via copy-on-write pointers has a couple of drawbacks:
     - Complex histogram API.
     - No good way to check for shared axis.
     - Cumbersome client code for maintaining sharing.
   - Could support histograms as the smallest element if we can do without sharing the axis.
     - Some memory overhead.
     - Compute overhead if all histograms have the same axis.

## Dataset overview

This prototype is trying to take inspiration from `xarray` and adopts its `Dataset` type to C++ and tailors it to the needs of neutron data handling.
The goal is not to simply copy `xarray`, but rather an attempt to see where solutions found by `xarray` can help us to solve our problems.

A `Dataset` as discussed here contains:

- A number of named dimensions.
- A number of "columns", i.e., vector-like data.
  Each column has an associated set of dimensions.
  This defines the length of the column.

As in `xarray`, the idea is to provide a number of basic operations for `Dataset`.
This would probably include basic arithmetic operations as well as "shape" operations such as slicing, extracting, and merging in one or multiple dimensions.
The basic operations are currently not prototyped yet.

As discussed elsewhere, holding each column with its concrete type is problematic.
Therefore, columns are type-erased.
For operations that are not part of the pre-defined basic set the underlying data needs to be accessed.
We have several options:

1. Access individual columns, by requesting a column of a certain type (optionally with a certain name).
   `Dataset` can then perform a lookup among its columns and cast the matching column to the requested concrete type.
1. Use some sort of view to iterate over one or more columns of concrete type.
   Essentially this is a convenience layer on top of option 1.) that acts as if data was stored as an array of structures.
   Furthermore it provides an elegant way of dealing with columns that do not share all their dimensions.
   - The view/iterator could iterate either all (applicable) dimensions, a specific dimensions, or all but certain dimensions, depending on the needs to the specific client code.
1. Access a subset of a `Dataset` at a given axis value for a given dimension.
   This is basically slicing of the `Dataset`.
   It is currently unclear how this should be handled and is currently not part of the prototype.
   The result would be a `Dataset` (implies that a copy of the data is returned) or a view into the `Dataset`.
   In either case, the result would still be type-erased.
   For any operation that subsequently needs to convert to concrete column types, this access mode is thus too inefficient if the dimension used for slicing is large, such as for spectrum numbers.

## Discussion points

1. Providing a set of basic operations for `Dataset` implies that a lot of code should operate with `Dataset`, in contrast to a (hypothetical or real) lower-level abstraction such as `Histogram`.
   - Is this reasonable?
   - Does one exclude the other?
1. Given the importance of histograms in our data reduction we should probably provide a `Histogram` type.
   As discussed in the FAQ above, storing histograms in `Dataset` has disadvantages.
   This could be solved by having a lightweight dummy histogram column for accessing the underlying data.
   `Dataset` would contain an extra column containing a new class `Histogram` as shown in `../src/type-erased-prototype/test/histogram_test.cpp`.
   Apart from the X axis being `const` this could act very similar to our current `Histogram` class.
   It can be used to directly modify the data within the `Dataset`.
   Copying `Histogram` extracts the data into an internal buffer, such that `Histogram` can be used also stand-alone without an attached `Dataset`.
   - Is this reasonable?
   - What are the major and minor drawbacks?
