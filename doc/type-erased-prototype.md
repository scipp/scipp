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
   - See [`test/dataset_test.cpp`](../src/type-erased-prototype/test/dataset_test.cpp) for examples.
1. Use some sort of view to iterate over one or more columns of concrete type.
   Essentially this is a convenience layer on top of option 1.) that acts as if data was stored as an array of structures.
   Furthermore it provides an elegant way of dealing with columns that do not share all their dimensions.
   - The view/iterator could iterate either all (applicable) dimensions, a specific dimensions, or all but certain dimensions, depending on the needs to the specific client code.
   - See [`test/dataset_iterator_test.cpp`](../src/type-erased-prototype/test/dataset_iterator_test.cpp) for examples.
1. Access a subset of a `Dataset` at a given axis value for a given dimension.
   This is basically slicing of the `Dataset`.
   It is currently unclear how this should be handled and is currently not part of the prototype.
   The result would be a `Dataset` (implies that a copy of the data is returned) or a view into the `Dataset`.
   In either case, the result would still be type-erased.
   For any operation that subsequently needs to convert to concrete column types, this access mode is thus too inefficient if the dimension used for slicing is large, such as for spectrum numbers.

## Discussion points

1. Providing a set of basic operations for `Dataset` implies that a lot of code should operate with `Dataset`, in contrast to a (hypothetical or real) lower-level abstraction such as `Histogram`.
   In general, I have the feeling that using datasets requires thinking a bit differently from the current way of working with workspaces, but I have trouble describing why that is.
   - Is this reasonable?
   - Does one exclude the other?
1. Given the importance of histograms in our data reduction we should probably provide a `Histogram` type.
   As discussed in the FAQ above, storing histograms in `Dataset` has disadvantages.
   This could be solved by having a lightweight dummy histogram column for accessing the underlying data.
   `Dataset` would contain an extra column containing a new class `Histogram` as shown in [`test/histogram_test.cpp`](../src/type-erased-prototype/test/histogram_test.cpp).
   Apart from the X axis being `const` this could act very similar to our current `Histogram` class.
   It can be used to directly modify the data within the `Dataset`.
   Copying `Histogram` extracts the data into an internal buffer, such that `Histogram` can be used also stand-alone without an attached `Dataset`.
   - Is this reasonable?
   - What are the major and minor drawbacks?

## Notes

- Units would be stored as a flag of a column.
  Operations with such columns would operate on these flags.
  Likewise, if a `Histogram` wrapper as discussed above is provided, it could handle units (this implies that operations with `Histogram` that change the unit cannot be done in-place).
- The prototype currently accesses columns and iterator elements by type.
  We will probably need to use a tag or ID instead:

  ```cpp
  // All possible variables given by enum. Need to support also generic
  // variables such as Double or String.
  enum class Variable { DetectorPosition, SpectrumPosition };

  // Type for variable is defined at compile time.
  template <Variable Var> struct variable_type;
  template <>
  struct variable_type<DetectorPosition> {
    using type = std::vector<Eigen::Vector3d>;
  };
  template <Variable Var>
  using variable_type_t = typename variable_type<Var>::type;

  class Dataset {
  public:
    // Dataset::get<Variable::DetectorPosition>() returns std::vector<Eigen::Vector3d>.
    template <Variable Var>
    const variable_type_t<Var> &get() const;
    // TODO how can we request const access (also for iterator?).
    // The natural way does not work since we have a non-type template parameter:
    // Dataset::get<const Variable::DetectorPosition>();
  };
  ```
  Additionally access via a name will probably be provided.
- We will need to support "axis" columns that are bin edges, i.e., are by 1 longer than the size of the dimension.
  If we iterate over all dimensions, what should the iteration for bin edges do?
  - Not possible?
  - Support iterating "bins", a virtual column providing center, left edge, right edge, and width?
    ```cpp
    Dataset d {Variable::BinEdge, Variable::DataPoint};
    // Full iteration with no core-dimensions, i.e., we are not iterating histograms.
    // This should fail to compile, since the Variable::BinEdge has a different length!
    DatasetIterator<Variable::BinEdge, Variable::DataPoint> it(d);
    // Use this instead (note that Variable::Bin is a virtual variable!)
    DatasetIterator<Variable::Bin, Variable::DataPoint> it(d);
    ```
- The time-of-flight axis in our workspaces can have a different length for each spectrum.
  This is a case that is not supported by `xarray` but we have two choices:
  - Do not handle time-of-flight as a dimension.
    This implies that the elements in our data columns would be vectors, basically the current `HistogramData::HistogramY`, etc.
  - Support columns with non-constant size in a certain dimension, e.g., bin edges and counts that have a different length in the time-of-flight dimension for every point in the spectrum dimension.
    Without having tried, this should be possible without too much trouble (provided that we do not require to *change* the individual lengths) and would only prevent certain operations, such as selecting a time-of-flight slice.
- For more convenient handling of `Dataset`, it could be beneficial to combine value and error into `struct DataPoint { double value; double error; };`.
  This avoid difficulties related to handling two separate columns for values and errors, in particular the need to always access two columns at the same time and the need for a mechanism linking a specific value column to its error column.
- Columns that are used as an axis to index into the `Dataset` should *not* be verified on write.
  This has been implemented for ensuring unique spectrum numbers as part of `IndexInfo`.
  The result is code that is probably hard to maintain, in particular since it required changes outside `IndexInfo`, in particular `ExperimentInfo`, `MatrixWorkspace`, and `ISpectrum`.
  Instead, do verification lazily only when an axis is actually used for accessing the `Dataset`.
  This results in some overhead (and MPI communication), but is does not appear to happen too frequently and should not be an issue for performance.
- Probably copy-on-write-wrapping each column makes sense.
- To avoid a fair number of complications, columns should ideally not contain any data apart from the individual items.
  If they do, the operations on single items must probably rely on certain assumptions?
  - Most prominent example: Unit for bin edges and data points.
    Where do we store it?
  - Can the unit be a separate column?
    - Any operation on `Dataset` that wants to handle units could simply handle it separately?
    - Still link to unit from `Histogram`?
    - Enforce unit column to be zero-dimensional?
  - Is the dimension effectively the same as a unit?
    - `Dimension::Tof` could be used to imply that the unit is time-of-flight microseconds, converting the unit could be done by changing the dimension label to, e.g., `Dimension::dSpacing`.
    - How can we assign a unit to the values of the dependent quantities (which do not have a dimension assigned).
- Do we need to link columns?
  For example, columns replacing `SpectrumInfo` would need a grouping column and columns replacing `DetectorInfo`.
  How can they access it efficiently?
  - Store pointer to other columns?
    Prevent removing columns if there are any references to it.
- We should definitely investigate how `Dataset` could support better cache reuse by chaining several operations or algorithms.
  - At runtime or compile-time?
  - Do we need things like expression templates to make this work?
    Does not play nicely with `auto`.
  - Is it worth the effort, given that we may often be limited by I/O?

## Example

A typical `Dataset`, equivalent to one of our current workspaces could contain the following columns and dimensions:

- Experiment logs and sample with (typically) no attached dimension.
- `Dimension::Detector`, applies to columns `DetectorInfo` (or an equivalent representation based on individual independent columns for positions, rotations, ...) and `DetectorIDs`.
- `Dimension::ScanIndex`, for scanning instrument (sync scan only), applies to `Position` and `Rotation` columns (currently part of `DetectorInfo`) and `ScanIntervals`.
- `Dimension::Spectrum`, applies to all columns that depend on the spectrum (but not the detector). Key examples are `SpectrumNumbers`, `SpectrumDefinitions` (mapping from spectrum to one or multiple detectors), `SpectrumInfo` (or an equivalent representation based in individual independent columns for positions, rotations, ...), `EventLists`, and histogrammed data.
- `Dimension::Tof`, applies mainly to columns X, Y, and E of histogrammed data, but see also discussion above regarding variable-length histograms (`Tof` is probably a bad name since it may be anything derived from it?).
- Multiple masking could be supported, e.g., for both `Dimension::Detector` to mask detectors as well as for `Dimension::Spectrum` and `Dimension::Tof` for masking (bins of) spectra.
  Should we have first-class masking support, i.e., as a feature built into `Dataset` that is handled automatically in many cases?

## Implementing algorithms for Dataset

Relevant section in `xarray` documentation: [ufunc for `xarray.Dataset`](http://xarray.pydata.org/en/stable/computation.html#math-with-datasets), in particular "core dimensions" are relevant for us since many operations such as those based on histograms have time-of-flight as core dimension.

Cases to consider:

- Apply algorithm based on function working on single item of single column?
- Apply algorithm based on function working on single column.
- Apply algorithm based on function working on multiple columns.
- Apply algorithm based on function working on iterator item.
  - Call wrapper must set unit of column based on that of individual returned items (and check consistency).
- Apply algorithm based on function working on `Dataset`.
- In-place operation.

What should algorithm return?

- Work in place as much as possible, adding columns if output is of different type?
- Return a copy with output replacing existing columns (or added as new columns), relying on copy-on-write for columns to avoid copy overhead?
- Return a single column (something like `xarray.DataArray`), or a `Dataset` containing only a single column?
  This would lose the connection to the instrument etc.!

Keep things elementary as long as possible.
For example, low-level algorithms should not support selecting indices, rather extracting a subset should be a separate step or be provided by the wrapper via a generic selection method.

### Requirement for new concepts and working methodology?

A `Dataset` is meant to allow for very generic containers replacing workspaces.
This has a couple of consequences that may require partially rethinking our algorithm concept.

`Dataset` can contain multiple columns containing data such as histograms.
For example, someone could decide to store histograms for a sample run and a can run in the same `Dataset` as two different columns.
Thus we need to be able to:
- Apply operations and algorithms to all columns (of a given type).
- Provide a mechanism to select at runtime which columns to apply an algorithm to.

Need a way to define mapping from input column to functor argument *without the need to have the algorithm work on the dataset level*?
Can we translate type -> list of related column Ids?

Do we need to describe algorithms at a higher level (similar to what the current `Algorithm::exec` does, but without being mixed with the lower levels?
- For simple algorithms without overloads we can provide the automatic way, others just operate on `Dataset`.
- How can we avoid the requirement to cast to explicit overloads, etc.?
  - `apply_to_all_columns<Histogram>(&Alg::rebin);` We specify the column type to apply to. This is used to select the overload.

```cpp
// Apply functor Alg to all matching columns, fails to compile if Alg has overloads.
apply<Alg>(d);

// Apply functor Alg to all columns containing histograms. Overload selected based on explicit given type.
apply<Alg, Histogram>(d);

// Apply functor Alg to column identified by name. Overload selected based on type of named column?
apply<Alg>(d, "my-column");

// Select overload based on runtime content of Dataset
if (d.contains<BinMask>())
  apply<Alg, Histogram, const BinMask>(d);
else
  apply<Alg, Histogram>(d);
```
