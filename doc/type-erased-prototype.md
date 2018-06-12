# Type-erased dataset prototype

## Nomenclature

This document uses the same nomenclature as `xarray` where applicable.
In particular:

- `Dataset` contains *variables*.
  Variables can mostly be imagined as columns of a table (a previous version of this document was therefore using the term column).

## FAQ

1. Why not workspaces with virtual interface to provide common functionality?
   - Common virtual interfaces are already running into trouble with the current workspaces.
     With more and more generic workspace types we would not be getting anywhere.

1. Why not with static typing of variables, i.e., similar to `std::tuple<Ts...>`?
   - Yields an unreasonably large number of workspace types that probably cannot be handled efficiently in terms of compile times, code size, and complete Python exports.
   - Does not cover cases where we require flexibility at runtime, such as for generic tables.

1. Why not units in type?
   - Yields a large number of variable types that is probably too large to handle.
   - Requires templating all low-level client code.

1. Why not a nested structure of table-like objects?
   - Apart from an easier implementation of methods for selecting certain slices from a `Dataset`, multi-level data structures do not seem to provide advantages and only increases complexity.

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
- A number of *variables*, i.e., vector-like data.
  Each variable has an associated set of dimensions.
  This defines the length of the variable.
  Scalar data (data with zero dimensions) is represented by a vector of length 1.

As in `xarray`, the idea is to provide a number of basic operations for `Dataset`.
This would probably include basic arithmetic operations as well as "shape" operations such as slicing, extracting, and merging in one or multiple dimensions.
The basic operations are currently not prototyped yet.

As discussed elsewhere, holding each variable with its concrete type is problematic.
Therefore, variables are type-erased.
For operations that are not part of the pre-defined basic set the underlying data needs to be accessed.
We have several options:

1. Access individual variables, by requesting a specific variable (identified by a tag) of a certain type (optionally with a certain name).
   `Dataset` can then perform a lookup among its variables and cast the matching variable to the requested concrete type.
   - See [`test/dataset_test.cpp`](../src/type-erased-prototype/test/dataset_test.cpp) for examples.
1. Use some sort of view to iterate over one or more variables of concrete type.
   Essentially this is a convenience layer on top of option 1.) that acts as if data was stored as an array of structures.
   Furthermore it provides an elegant way of dealing with variables that do not share all their dimensions.
   - The view/iterator could iterate either all (applicable) dimensions, a specific dimensions, or all but certain dimensions, depending on the needs to the specific client code.
   - See [`test/dataset_iterator_test.cpp`](../src/type-erased-prototype/test/dataset_iterator_test.cpp) for examples.
1. Access a subset of a `Dataset` at a given axis value for a given dimension.
   This is basically slicing of the `Dataset`.
   It is still unclear how this should be handled in detail and is currently not part of the prototype.
   The result would be a `Dataset` (implies that a copy of the data is returned) or a view into the `Dataset`.
   In either case, the result would still be type-erased.
   For any operation that subsequently needs to convert to concrete variable types, this access mode is thus too inefficient if the dimension used for slicing is large, such as for spectrum numbers.

As described in this document, `Dataset` should be able to replace all of our workspace types, with maybe the exception of `MDEventWorkspace`.
This includes not just `MatrixWorkspace` and its child classes but also `TableWorkspace`, `PeaksWorkspace`, and `MDHistoWorkspace`.
`Dataset` would provide a uniform interface in a single type for all of these.
Furthermore, `Dataset` will cover many other cases that are currently impossible to represent in a single workspace.

## Details and discussion

- Probably copy-on-write-wrapping each variable makes sense.
  - Should be handled all internally, i.e., types used as variables do not need to support this on their own.

- Given the importance of histograms in our data reduction we should provide a `Histogram` type.
  As discussed in the FAQ above, storing histograms in `Dataset` has disadvantages.
  This could be solved by having a lightweight dummy histogram variable for accessing the underlying data.
  `Dataset` would contain an extra variable containing a new class `Histogram` as shown in [`test/histogram_test.cpp`](../src/type-erased-prototype/test/histogram_test.cpp).
  Apart from the X axis being `const` this could act very similar to our current `Histogram` class.
  It can be used to directly modify the data within the `Dataset`.
  Copying `Histogram` extracts the data into an internal buffer, such that `Histogram` can be used also stand-alone without an attached `Dataset`.
  - Is this reasonable?
  - What are the major and minor drawbacks?

- The prototype currently accesses variables and iterator elements by type.
  We will probably need to use a tag or ID instead:

  ```cpp
  // Need to support also generic variables such as Double or String.
  // Cannot use enum since it will not let us specify const access.
  // enum class Variable { DetectorPosition, SpectrumPosition };
  struct Variable {
    struct DetectorPosition {};
    struct SpectrumPosition {};
  };

  // Type for variable is defined at compile time.
  template <class Tag> struct variable_type;
  template <>
  struct variable_type<Variable::DetectorPosition> {
    using type = std::vector<Eigen::Vector3d>;
  };
  template <>
  struct variable_type<const Variable::DetectorPosition> {
    using type = const std::vector<Eigen::Vector3d>;
  };
  template <class Tag>
  using variable_type_t = typename variable_type<Tag>::type;

  class Dataset {
  public:
    // Support const-correct access to play nicely with copy-on-write mechanism:
    // Dataset::get<Variable::DetectorPosition>() returns std::vector<Eigen::Vector3d> &.
    // Dataset::get<const Variable::DetectorPosition>() returns const std::vector<Eigen::Vector3d> &.
    template <class Tag>
    variable_type_t<Tag> &get() conditional_const;
  };
  ```
  Additionally access via a name will probably be provided.

- We will need to support "axis" variables that are bin edges, i.e., are by 1 longer than the size of the dimension.
  If we iterate over all dimensions, what should the iteration for bin edges do?
  - Not possible?
  - Support iterating "bins", a virtual variable providing center, left edge, right edge, and width?
    ```cpp
    Dataset d {Variable::BinEdge, Variable::DataPoint};
    // Full iteration with no core-dimensions, i.e., we are not iterating histograms.
    // This should fail to compile, since the Variable::BinEdge has a different length!
    DatasetIterator<Variable::BinEdge, Variable::DataPoint> it(d);
    // Use this instead (note that Variable::Bin is a virtual variable!)
    DatasetIterator<Variable::Bin, Variable::DataPoint> it(d);
    ```
    We may have bin edges in more than one dimension, how do we name those variables and distinguish them (think, e.g., of transposing a workspace containing histograms, yielding a bin-edge axis for the "Y" axis)?
    - Is `Variable::BinEdge` actually a reasonable name?
      Should it be `Variable::TofEdge` (and `Variable::TofBin`) instead?
      This would imply changing variable labels if units are changed, yielding, e.g., `Variable::dSpacingEdge`.
      How would we handle this generically in client code accessing the edges, given that most code will not care whether it is `Tof` or `dSpace` or anything else?
      Would it make sense to support `Variable::Point`, `Variable::BinEdge`, and `Variable::Bin` as aliases for whatever the current unit is, e.g., `Variable::Tof`, `Variable::TofEdge`, and `Variable::TofBin`?

- The time-of-flight axis in our workspaces can have a different length for each spectrum.
  This is a case that is not supported by `xarray` but we have two choices:
  - Do not handle time-of-flight as a dimension.
    This implies that the elements in our data variables would be vectors, basically the current `HistogramData::HistogramY`, etc.
    This does not feel like a good solution to me?
  - Support variables with non-constant size in a certain dimension, e.g., bin edges and counts that have a different length in the time-of-flight dimension for every point in the spectrum dimension.
    Without having tried, this should be possible without too much trouble (provided that we do not require to *change* the individual lengths) and would only prevent certain operations, such as selecting a time-of-flight slice.
    - Need a (hidden?) index variable that provides the start of data for each new histogram.
      Can we simply use the `Histogram` variable, given that it stores offsets to the underlying data anyway?

- For more convenient handling of `Dataset`, it might be beneficial to combine value and error into `struct DataPoint { double value; double error; };`.
  This avoids difficulties related to handling two separate variables for values and errors, in particular the need to always access two variables at the same time and the need for a mechanism linking a specific value variable to its error variable.
  There are a couple of non-negligible disadvantages:
  - Changes/complicates the way of interaction with other libraries such as `numpy` and `gsl`.
    - Vanilla `numpy` does not appear to have good support for arrays of value/error tuples, i.e., this does not appear to be a good solution.
  - Would make vectorization more difficult.
  - Given the disadvantages of storing values and errors as `DataPoint`, we have the option to keep value and error separate in the `Dataset`, but can nevertheless provide access via `DataPoint` for convenience:
    ```cpp
    DatasetView<Variable::DataPoint> view(dataset);
    // Cannot get by reference, so getter returns by value
    DataPoint p = view[0].dataPoint();
    // No reference, i.e., we cannot provide assignment, use setter
    view[0].setDataPoint(DataPoint(value, error));
    ```

- Variables that are used as an axis to index into the `Dataset` should *not* be verified on write.
  This has been implemented for ensuring unique spectrum numbers as part of `IndexInfo`, which was introduced as part of refactoring for supporting scanning instruments and MPI.
  The result was code that is probably hard to maintain, in particular since it required changes outside `IndexInfo`, in particular `ExperimentInfo`, `MatrixWorkspace`, and `ISpectrum`.
  - Instead, do verification lazily only when an axis is actually used for accessing the `Dataset`.
  - Results in some overhead (and MPI communication), but does not appear to happen too frequently and should not be an issue for performance.

- A dimension in the `Dataset` is effectively the same as a unit.
  For example,
  - `Dimension::Tof` could be used to imply that the unit is time-of-flight microseconds, converting the unit could be done by changing the dimension label to, e.g., `Dimension::dSpacing`.
  - As a consequence, any operation that checks matching dimensions of variables will implicitly ensure matching units.

- To avoid a fair number of complications, variables should ideally not contain any data apart from the individual items.
  If they do, the operations on single items must probably rely on certain assumptions?

- Mantid currently does not handle units for variables such as Y and E in our current Mantid nomenclature, with a partial exception:
  Distinguishing between "histogram data" and "distribution data", which is effectively the difference between a dimensionless quantity (counts) and the dimensionful frequency (unit is 1/x-unit).
  Handling units for Y and E would allow us to:
  - Get rid of the distinction between histogram data distribution data.
  - Provide a way to distinguish standard deviations and variances.
  - Allow more general operations that yield different units.

  As mentioned above, storing units in the type (using `boost::units`) is not really feasible.
  We must thus provide a runtime-equivalent at a higher level.
  The problem of having the unit at a higher level is that it can be ignored.
  - This is the same issue that `HistogramData::Histogram` attempted but failed to solve:
    At a high level `Histogram` knows that it stores standard-deviations and the API can return variances if required.
    However, low level access is sometimes used to square the internal error values, such that there is a mismatch between the actual content and the "guarentee" that `Histogram` gives.
    Without storing compile-time quantities this is a problem that cannot be solved, so we will have to live with it.

  There are a couple of options for having units attached to a variable in a `Dataset`:
  - A variable could be a quantity, basically something like `std::pair<UnitTag, std::vector<double>>`.
    - Not all variables would need to have a unit, but common ones such as Y and E would.
  - The unit tag could be a separate variable:
    - Any operation on `Dataset` that wants to handle units could simply handle it separately?
    - Enforce unit variable to be zero-dimensional?

  `Variable::Histogram` should link to the unit such that operations with histogram are safe.
  - The unit link would then handle things like preventing adding a time-of-flight histogram to a d-spacing histogram).
  - Operations with histograms will have only const access to the unit (since there are many histograms but just one unit in the `Dataset`), preventing in-place operation with histograms if the unit changes.
    However, there may just be a handful of unit-changing operations (multiplication, unit conversion, converting errors between standard deviations and variances) that would need to be implemented explicitly, so maybe this is not an issue?

- We probably need to link variables together.
  For example, variables replacing `SpectrumInfo` would need a grouping variable and variables replacing `DetectorInfo`.
  - Store pointer to other variables?
  - Prevent removing variables from `Dataset` if there are any references to it from within the same `Dataset` instance.
  - Link value variables to their error variables and vice versa (unless stored as `DataPoint` variables, see above).

- We should definitely investigate how `Dataset` could support better cache reuse by chaining several operations or algorithms.
  - At runtime or compile-time?
  - Do we need things like expression templates to make this work?
    Does not play nicely with `auto`.
  - Is it worth the effort, given that we may often be limited by I/O?
  - Probably not too hard to implement for anything that can operate with a wrapper of a fixed signature.

- For variables that do not explicitly store their data (such as variables from `SpectrumInfo`, which compute them based on items in variables from `DetectorInfo`) returning a reference to individual items from the iterators/views is not possible.
  - For `const` access we can simply return by value.
    Attempts to assign to a non-`const` reference will result in compilation failure, protecting us from potential bugs due to the slight interface inconsistency.
  - Non-`const` access is not possible in this way so we need to provide a different solution.
    However, this case is quite rare and should therefore not be a show stopper.
    Currently the only example we are aware of is masking in `SpectrumInfo`, which currently masks all associated detectors.

- Monitors can be stored as (a set of) separate variable(s) instead of the current mechanism linking to a separate workspace.

## Example

A typical `Dataset`, equivalent to one of our current workspaces could contain the following variables and dimensions:

- Experiment logs and sample with (typically) no attached dimension.
- `Dimension::Detector`, applies to variables `DetectorInfo` (or an equivalent representation based on individual independent variables for positions, rotations, ...) and `DetectorIDs`.
- `Dimension::ScanIndex`, for scanning instrument (sync scan only), applies to `Position` and `Rotation` variables (currently part of `DetectorInfo`) and `ScanIntervals`.
- `Dimension::Spectrum`, applies to all variables that depend on the spectrum (but not the detector). Key examples are `SpectrumNumbers`, `SpectrumDefinitions` (mapping from spectrum to one or multiple detectors), `SpectrumInfo` (or an equivalent representation based in individual independent variables for positions, rotations, ...), `EventLists`, and histogrammed data.
- `Dimension::Tof`, applies mainly to variables X, Y, and E of histogrammed data, but see also discussion above regarding variable-length histograms (`Tof` is probably a bad name since it may be anything derived from it, but see discussion on units above).
- Multiple masking could be supported, e.g., for both `Dimension::Detector` to mask detectors as well as for `Dimension::Spectrum` and `Dimension::Tof` for masking (bins of) spectra.
  - Should we have first-class masking support, i.e., as a feature built into `Dataset` that is handled automatically in many cases?
    It is not really clear what this means.
  - Reversible masking has been requested.
    Can achieve that by taking into account masking variables, e.g., when plotting or fitting data.
    Only certain operations operate in a way that makes masking irreversible.
    They key example is `Rebin`, which, when given a `Dataset` containing `Histogram` and `BinMask` variables, could return a `Dataset` without masking since it was irreversibly absorbed into the data.

## Interface mock-up

Naming is just for illustration, may be very different in the actual implementation.

```cpp
Dataset d;

// Imagine arbitrary initialization here

// Variables can be added (or removed) at any time.
std::vector<bool> mask(d.size(Dimension::SpectrumNumber), false);
d.add<Variable::Mask>(mask, Dimension::SpectrumNumber, "ROI");
d.remove(Variable::EventList);
// Note: Certain variables are tied together and are added/removed "atomically",
// e.g., instrument, detector info, spectrum info, and mapping to detectors.

// Dimensions can be added at any time.
// Add axis with new dimension
d.add<Variable::Polarization>({Spin::Up, Spin::Down}, Dimension::Polarization, "pol");
// Extend the histogram variable into the new dimension (data zero initialized?).
d.extend(Variable::Histogram, Dimension::Polarization);

// Copy constructor. All variables are automatically shared.
auto d_copy(d);

// Get variable by label
const auto &positions = d.get<const Variable::DetectorPosition>(); // Dataset keeps sharing
auto &positions = d.get<Variable::DetectorPosition>(); // triggers copy-on-write

// Slicing and extracting datasets (details unclear currently, but things
// like this will be supported in one way or another):
auto signal = d.slice<Variable::Polarization>(Spin::Up)
              - d.slice<Variable::Polarization>(Spin::Down);
// The above could potentially use (read-only) views on the right-hand
// side and thus avoid copies. Can slice into a "vector" of views,
// indexed by axis used for slicing:
auto slices = d.slices<Variable::Polarization>();
auto signal = slices[Spin::Up] - slices[Spin::Down];

// Slicing into Datasets is inefficient for long axes such as Dimension::Spectrum
// since variables in Dataset are type-erased. Use typed iterator or view for
// subset of variables instead:
DatasetView<Variable::SpectrumNumber, Variable::SpectrumPosition> view(d);
view->begin().get<Variable::SpectrumNumber>() = 17;
// Could provide named convenience methods for standard variables.
view->begin()->spectrumNumber() = 17;
view[42].spectrumPosition() *= -1.0;
// Providing these convenience methods would allow for generic code, supporting
// Datasets that store different representations of the same data. Client code
// would simply template on DatasetView<T> (or just T).
// If it is not possible to return a reference to the underlying data, e.g.,
// when it is computed on access, it is returned by value and assignment is not
// possible (could potentially generate a setter).

// Use DatasetView to operate on variables with different dimensions, anything
// that does not share all dimensions must be const:
DatasetView<const Variable::SpectrumMask, Variable::Value, Variable::Error> view(d);
// Note: DatasetView<A, B> and DatasetView<B, A> will be the same type since the
// order is irrelevant and client code should not be burdened with using a
// special order of types.
for (auto &item : view) { // Iterates, e.g., Dimension::Spectrum and Dimension::Tof
  if (item.get<const Variable::SpectrumMask>()) { // Returns the same regardless of the Tof position of the iterator.
    item.value() = 0.0;
    item.error() = 0.0;
  }
}

// Can exclude some dimensions from iteration. Variables that depend on the
// excluded dimensions need to be accessed as "slabs" extending into those
// dimensions.
DatasetView<const Variable::SpectrumMask, Slab<Variable::Value>,
            Slab<Variable::Error>> view(d, DoNotIterate::Tof);
for (auto &item : view) { // Iterates, e.g., Dimension::Spectrum
  if (item.get<const Variable::SpectrumMask>()) {
    item.get<Slab<Variable::Value>>() = 0.0; // Assigns to everything in Slab of Dimension::Tof.
    item.get<Slab<Variable::Error>>() = 0.0;
    // Plural versions of the named accessors might be provided for accessing slabs:
    item.values() += 1.0;
  }
}

// Binary operations
// - Check for matching dimensions (broadcasts if possible, e.g., when adding a single value).
// - Check for matching axis variables (spectrum numbers, bin edges, ...).
// - Add all non-axis variables.
// - TODO: Figure out matching details and behavior for missing variables.
d1 += d2;
```

## Implementing algorithms for Dataset

Relevant section in `xarray` documentation: [ufunc for `xarray.Dataset`](http://xarray.pydata.org/en/stable/computation.html#math-with-datasets), in particular "core dimensions" are relevant for us since many operations such as those based on histograms have time-of-flight as core dimension.

Cases to consider:

- Apply algorithm based on function working on single item of single variable?
- Apply algorithm based on function working on single variable.
- Apply algorithm based on function working on multiple variables.
- Apply algorithm based on function working on iterator item.
  - Call wrapper must set unit of variable based on that of individual returned items (and check consistency).
- Apply algorithm based on function working on `Dataset`.
- In-place operation.

What should algorithm return?

- Work in place as much as possible, adding variables if output is of different type?
- Return a copy with output replacing existing variables (or added as new variables), relying on copy-on-write for variables to avoid copy overhead?
- Return a single variable (something like `xarray.DataArray`), or a `Dataset` containing only a single variable?
  This would lose the connection to the instrument etc. and does not seem very useful!

Keep things elementary as long as possible.
For example, low-level algorithms should not support selecting indices, rather extracting a subset should be a separate step or be provided by the wrapper via a generic selection method.

### Requirement for new concepts and working methodology?

A `Dataset` is meant to allow for very generic containers replacing workspaces.
This has a couple of consequences that may require partially rethinking our algorithm concept.

`Dataset` can contain multiple variables containing data such as histograms.
For example, someone could decide to store histograms for a sample run and a can run in the same `Dataset` as two different variables.
Thus we need to be able to:
- Apply operations and algorithms to all variables (of a given type).
- Provide a mechanism to select at runtime which variables to apply an algorithm to.

If we want to "automatically" create algorithms from low-level code that does not know about `Dataset`, we need a way to define mapping from input variable to functor argument *without the need to have the algorithm work on the dataset level*?
Can we translate type -> list of related variable Ids?

Do we need to describe algorithms at a higher level (similar to what the current `Algorithm::exec` does, but without being mixed with the lower levels?
- For simple algorithms without overloads we can provide the automatic way, others just operate on `Dataset`.
- How can we avoid the requirement to cast to explicit overloads, etc.?
  - `apply_to_all_variables<Histogram>(&Alg::rebin);` We specify the variable type to apply to. This is used to select the overload.

```cpp
// Apply functor Alg to all matching variables, fails to compile if Alg has overloads.
apply<Alg>(d);

// Apply functor Alg to all variables containing histograms. Overload selected based on explicit given type.
apply<Alg, Histogram>(d);

// Apply functor Alg to variable identified by name. Overload selected based on type of named variable?
apply<Alg>(d, "my-variable");

// Select overload based on runtime content of Dataset
if (d.contains<BinMask>())
  apply<Alg, Histogram, const BinMask>(d);
else
  apply<Alg, Histogram>(d);
```
