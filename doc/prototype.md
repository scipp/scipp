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
  We should look into ways of doing type-erasure on intermediate levels of a workspace table structure.
  - How can we keep the benefits of both worlds, type system in C++, as well as flexible containers (workspaces) assembled from arbitrary types?
    In C++ we still want to be able to write natural C++-style code, where client code using workspaces can work with types and does not need to rely on casting or cumbersome virtual interfaces.
