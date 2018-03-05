# Prototype 1

## Done

See mainly [`test/test.cpp`](../src/prototype/test/test.cpp), [`algorithms.h`](../src/prototype/algorithms.h), and [`workspace.h`](../src/prototype/workspace.h) in `src/prototype/`.

A functor `Alg` with `apply` method defines an algorithm.
It is invoked with the help of a templated `call` wrapper.
Supported features are:

- Algorithms with single input workspace.
- If the constructor has arguments, all arguments but the first workspace are passed to the constructor.
- If the constructor has no arguments, all arguments but the first are passed to `Alg::apply` as second and subsequent arguments.
- The `call` wrapper takes the workspace by-value. In-place operation can be achieved by moving the input workspace.
- If `Alg::apply` accepts meta data components of the workspace it is invoked with that meta data as first argument.
- If `Alg::apply` accepts data items of the workspace it is invoked for all data items in the workspace as first argument.
- Mutating the type of the items in a workspace is supported, e.g., for `Rebin` converting `EventList` to `Histogram`.
- The workspace type can depend on more than just the data type, e.g., the instrument type (`SpectrumInfo`, `QInfo`, ...).
  Algorithm operating on other meta data or data items are unaffected and work without knowledge of that aspect of the workspace type.

## Potential problems

- Functor signatures that are not supported by call wrappers result in template error messages that would probably be too obscure for many developers, unless documented properly.
  Can we solve this by putting a static assert with a sensible error message in the place of failed instantiation?

## To do

- Should we support history in C++?
  If we have the ADS and property system only in Python, we do not have access to the string values of inputs.
  Could probably be solved in several ways, but it is not clear whether that is the right way to go.
- Support logging by having an optional logger argument in the functor constructor?
- Automatically do threaded execution if `Functor::apply` is const.
- What do we define Python exports for? The `call`-wrapped version of the functors?

# Prototype 2

## Existing technology

Considerations around support constant-wavelength workspaces, multi-period workspaces, imaging workspaces, parameter scans, and multi-dimensional workspaces show up similarities to what, e.g., [pandas](https://pandas.pydata.org/) does.

Other existing technology that we should investigate (most found [here](https://www.reddit.com/r/cpp/comments/64hzmd/is_there_a_dataframe_library_in_clike_pandas_in/)):

- [xarray](http://xarray.pydata.org/en/stable/)
- ROOT's [TNtuple](https://root.cern.ch/doc/master/classTNtuple.html) and [TDataFrame](https://root.cern.ch/doc/master/classROOT_1_1Experimental_1_1TDataFrame.html)
- [Apache Arrow](https://arrow.apache.org/#)
- [easyLambda](https://haptork.github.io/easyLambda/)

Investigation does not necessarily imply that we would use such a library.
However, it is very likely that they solved similar problems that we will be facing, so looking at their solutions and in particular their API could prove valuable.

## Done

See mainly [`test/data_frame_test.cpp`](../src/prototype2/test/data_frame_test.cpp) and [`data_frame.h`](../src/prototype2/data_frame.h) in `src/prototype2/`.

A couple of levels of nested table-like data structures seem to give a good representation of our workspace types.
The implemented example shows a type `DataFrame`, similar to `pandas.DataFrame`.
Columns are given by indexable types, so common types in Mantid are roughly given as follows, the first template argument being the axis:

```cpp
// Note: In the prototyped example types such as Counts are templated with their unit.
using Histogram = DataFrame<BinEdges, Counts, CountStdDevs>;

// missing instrument and meta data
using Workspace2D = DataFrame<std::vector<SpectrumNumber>,
                              std::vector<Histogram>, SpectrumInfo>;
// missing instrument and meta data
using EventWorkspace =
    DataFrame<std::vector<SpectrumNumber>, std::vector<EventList>,
              std::vector<BinEdges>, SpectrumInfo>;

// DetectorInfo is data of different length => workspace contains two DataFrames of different length!
using Instrument = DataFrame<std::vector<DetectorID>, DetectorInfo>;

using Workspace2D =
    std::tuple<DataFrame<std::vector<SpectrumNumber>, std::vector<Histogram>,
                         SpectrumInfo>,
               DataFrame<std::vector<DetectorID>, DetectorInfo>, Logs>;

// could handle multiple workspaces for same instrument like this:
// single data:
using Workspace2D = std::tuple<DataFrame<std::vector<SpectrumNumber>,
                                         std::vector<Histogram>, SpectrumInfo>,
                               Instrument, Logs>;
// multiple, sharing instrument:
// if something inside inner frame needs data/metadata, walk up in the hierarchy
// until found?
using Workspace2DMultiPeriod =
    std::tuple<DataFrame<std::vector<RunIndex>,
                         DataFrame<std::vector<SpectrumNumber>,
                                   std::vector<Histogram>, SpectrumInfo>,
                         Logs>,
               Instrument>;
```

## Problems

While the nested table structure feels reasonable, we are running into a massive problem if all columns and units are represented as types:
There is an enormous number of resulting histogram and workspace types.
This causes 2 problems:

1. The `call` wrappers frame the first prototype need to be instantiated for a large number of workspaces types, for a large number of algorithms.
   This leads to long compile times and large binary sizes.
2. We need Python exports, i.e., we actually do not know all required types and type combinations at compile time.
   We would thus need to attempt to explicitly instantiate all possible combinations (including all possible *combinations* of types and units, for algorithms working on more than one workspace).

Even without having tried, I believe the number of types is not manageable.
We thus need to look for different options:
- Do not store the units in the type.
  We could use an ID that is checked in for all operations.
  This would make using `boost::units` harder, since it is also based on templates and compile time.
  A proper unit system would be a big advantage, so we should try to find a solution that can make use of an existing units library such as `boost::units`.
- Even without units, we probably still have too many types.
  A [quick test](../src/prototype/test/compile_time_test.cpp) based on prototype 1:
  - Using a templated workspace type `Workspace<DataPoint, Dummy<N>>`.
  - Algorithm is agnostic w.r.t. `Dummy<N>`, i.e., it will be instantiated only once.
  - Call wrappers and workspace constructor are instantiated once for every `N`.
  - Result for 100 workspace types:
    - `1.3 seconds` extra compilation time.
    - `800 kByte` extra binary size.

    Given that the call wrappers will likely become more complicated in an actual implementation this may turn out to be even more.

  We should look into ways of doing type-erasure on intermediate levels of a workspace table structure.
  - How can we keep the benefits of both worlds, type system in C++, as well as flexible containers (workspaces) assembled from arbitrary types?
    In C++ we still want to be able to write natural C++-style code, where client code using workspaces can work with types and does not need to rely on casting or cumbersome virtual interfaces.
- If the unit is stored as a field in our `x`, `y`, and `e` arrays we can probably drop the distinction between `Counts` and `Frequencies` since the same information is encoded in the unit.
  - We lose a little bit of type safety since the unit field could be ignored.
    Given that currently a lot of our code is using direct access to the `std::vector<double>` within `Histogram`, i.e., bypasses the type system, we would not be considerable worse of that today.
  - We gain a factor of two in terms of number of workspace types to support.
  - We have to option to store variances instead of standard deviations in a workspace, the content of the uncertainties vector can be deduced from its unit.
    - Currently `Histogram` always stores standard deviations, but there is a fair amount of code that temporarily squares the contents, i.e., there is a mismatch between the type and the actual content.

# Dataset

Partially inspired by `xarray.Dataset`.

```cpp
// Somehow define dimensions:
// Spectrum:        x                          x                    x
// Tof:                              x         x
Dataset<std::vector<SpectrumNumber>, BinEdges, std::vector<Counts>, SpectrumInfo, Logs> s;

// Access via iterator or index (using the latter for simplicity of examples)
template <Dimension dim>
auto at(const gsl::index i) { /*...*/ }

// s.at<Dimension::Spectrum>(i) returns:
// Dataview<SpectrumNumber &, const BinEdges &, Counts &, SpectrumInfoItem &, const Logs &> 

// s.at<Dimension::Tof>(i) returns:
// Dataview<const std::vector<SpectrumNumber> &, BinItem &, Counts &, const SpectrumInfo&, const Logs&>
// - Counts has stride internally? Otherwise need to return it by value
// - does a non-const BinItem make sense?
```

How do we access and modify data that may or may not be inside a vector?
For example, `ConvertUnits` needs to modify `BinEdges`, which may either be a single item in the `Dataset` or a vector holding different `BinEdges` for each spectrum.
`ConvertUnits` should not need to know about the difference!
- Generally, we need a way to iterate all objects of a certain type, regardless of how nested they are within vectors. For example, we need to be able to iterate over a `Dataset` such that each iterator position references to a unique writable `Counts` object.
  This is the reverse of the templated `at` method above, where we specify the dimension.
  - Could use a flattened structure everywhere: Put all items in `Dataset` into a vector, no nesting supported.

# FlatDataset

A flattened version of `Dataset`, which seems to solve some problems discussed previously.
Note that this is a working title to distinguish it from the version above using nested vectors and would be renamed.
The current implementation is just a sketch and probably has some inefficiencies, it merely serves to prove the concept.

## Key advantages over `Dataset` and other data structures discussed above

- Shape and structure is *not* encoded in type anymore, contrary to examples prototyped above.
  - This may help to drastically reduce the number of distinct workspace types, solving parts of the problems discussed above.
  - Reduces the amount of template meta programming required to make algorithms work in a way similar to what is shown above in prototype 1.
- Makes iteration easier if the dimension does not matter, cf. discussion above in the section on `Dataset`.
  - Column access agnostic of potential underlying multi-dimensional structure is supported.
  - Row access based on index defined based on certain column is supported (a lot of details need to be sorted out), providing access also to other columns even if their dimensions do not match.

## To do

- Look more into `xarray` and in particular its link to the [Common Data Model](https://www.unidata.ucar.edu/software/thredds/current/netcdf-java/CDM/).
  - Is this something we should follow more closely?

## Features and details

### Dimensions

Common dimensions in workspaces could include:

- `SpectrumNumber`.
- `DetectorID` (carried by `DetectorInfo`, but typically no other variable in a dataset).
- `Q` in (a) one or (b) more dimensions, for equivalents of (a) `MatrixWorkspace` after `SofQW` or (b) `MDHistoWorkspace`.
- Parameters such as temperature.

Time-of-flight may be a special case, see below.

Furthermore, using dimensions we can support within a single workspace:

- Multiple runs / periods.
- Polarization.
- Multiple `E_i`, for example for RRM (repetition-rate multiplication) used at spectrometers.

Due to the flat structure, basic operations that are agnostic to these dimensions can simply iterate over all elements.
Access via an axis value in a certain dimension can allow for operations such as subtracting "spin-up" from "spin-down".

### Handling Histograms and time-of-flight data

For non-event neutron-count data or data derived from it, we need to support multiple cases:

1. Variable-length histograms, mimicking something like `std::vector<BinEdges>` and `std::vector<Counts>`, where the vector is over other dimensions such as spectrum numbers.
2. Fixed-length histograms, which can either use the same as case 1., or a unified multi-dimensional `BinEdges` and `Counts`.
3. Fixed-length histograms with shared bin edges, which can be implemented similar to 1. or 2.:
   1. `BinEdges` with `std::vector<Counts>`.
   2. `BinEdges` with `Counts`, where `BinEdges` has less dimensions than `Counts`.
4. Imaging data (or other) where we may *not* want time of flight as leading dimension, i.e., a stack of images.
   Can be supported by case 3.i, with a different ordering of dimensions.
5. Constant-wavelength data from reactor sources, which are not histograms but store a `Counts` objects as well.

To what extent can we hide the internal differences between these cases?
What are the problems of the current approach?

- The current way of sharing `BinEdges` is cumbersome.
  Modifying the `BinEdges` requires manual intervention in client code to preserve sharing.
- Checking whether `BinEdges` of histograms in a workspace are common is relying on fuzzy checking which can potentially be wrong.

#### Problems to solve

If we do not store data in a workspace as a vector of histograms, how to we pass such data a lower level code?

- Keep `Histogram` (or a modification of it) as a type for handling stand-alone histograms?
  Use it for setting data into a dataset and extracting from a dataset?
- Pass ranges instead of objects, similar how the `std::algorithm` library handles things?
  Modification could be in-place, or with a separate set of input and output iterators/ranges.
  - Ranges could be generalized to support spreads in iterations, such that we can operate on transposed data on-the-fly, without the need to perform an actual transpose.
- For variable-length histograms, it seems like having time-of-flight as a dimension of the dataset is not adequate?
  - Iteration over time-of-flight axis would not be sensible, since the axis depends on, e.g., the spectrum number.
  - Could still store all `Counts` and `BinEdges` concatenated in the same vector, but is there any advantage in that?

Client code that does not modify the bin edges can easily ignore the fact that bin edges may be shared.
Likewise, client code that modifies only bin edges can ignore the factor that multiple spectra may be sharing it.
However, there are two non-trivial cases:

- Code that breaks sharing of bin edges (`ConvertUnits`).
  - Can probably be handled in an automatic way: If code requests iteration with non-`const` `BinEdges`, we can extend `BinEdges` along the other dimensions, i.e., *implicitly* convert to non-shared `BinEdges`.
  - Alternatively we can require an *explicit* conversion to non-shared `BinEdges`.
    For low level algorithms this might actually be the preferred choice?
- Code that modifies bin edges as well as counts, but does not break sharing (are there any examples for this?).

How can we provide safe handling of units?

- If units are handled at runtime, we cannot attach units to each single data point.
  Thus, iterators and index-based access is bypassing the unit handling and is thus unsafe.
- If we handle units at compile time, we can correctly handle units in all operations using single data points.
  However, this brings us back to the problem of a large variety of workspaces types that is too large to handle.

### Iteration

Joint iteration over all variables in a dataset can be used to hide certain multi-dimensional aspects in case they are not relevant to the client code.
Considering a over all items given by a set of one or more dimensions.

- For all variables in the dataset that have a matching set of dimensions direct, possibly non-`const`, access to items of these variables can be provided.
- For all variables that have fewer dimensions, `const` access is provided. For example, iterating over all spectrum numbers can provide a `const` reference to the instrument or the sample logs.
- For all variables that have more dimensions, possibly non-`const` access can be provided via a slice containing the extra dimensions. For example, if we iterate over the polarization states, at each iterator position we can access the set of all histograms with that polarization state as a slice of the underlying larger block of histograms.

### Copy-on-write mechanism

Copy-on-write (COW) sometimes adds complications in client code (see the rather complex interface of `Histogram` and `MatrixWorkspace` dealing with setting up sharing) and puts limitations on threadability.

- Reduce the use of COW, unless the overhead is too large.
  - Histogram data will not benefit much if we provide means of having a shared X axis.
    Operations that copy a workspace while keeping data should maybe just be done in-place if memory overhead is an issue?
  - Logs can get large and are modified rarely, probably COW is useful here.
  - Instrument can get large and is modified rarely, probably COW is useful here.

### Units

A better support of units across Mantid should definitely be part of a design replacing existing workspaces.

- `BinEdges` and `Counts`, as well as instrument information such as positions in `DetectorInfo` should ideally all handle units.
  - Attaching units to `Counts` removes the need to distinguish between `Counts` and `Frequencies` (`Counts` may not be a good name anymore, this is just a working title, maybe `Values` and `Errors` to replace the current `Counts` and `CountStandardDeviations`?).
- Angles should have units, to avoid recurring bugs of dealing with radians and degrees.
