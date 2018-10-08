# Dataset design

- Author: Simon Heybrock
- Prototype review: Owen Arnold, Martyn Gigg
- Design review:

## Contents

- [Context](#context)
- [High level design overview](#overview)
  - [Components](#overview:components)
  - [Operations](#overview:operations)
  - [Relation to existing workspace types](#overview:relation-to-existing-workspace-types)
  - [Python exports](#overview:python-exports)
- [Design details](#details)
  - [Dependencies](#details:dependencies)
  - [Unit tests](#details:unit-tests)
  - [Benchmarks](#details:benchmarks)
  - [Documentation](#details:documentation)
  - [Scope and extension](#details:scope-and-extension)
  - [Naming](#details:naming)
  - [Copy-on-write mechanism](#details:copy-on-write-mechanism)
  - [Uncertainties are stored as variances, not standard deviations](#details:uncertainties-are-stored-as-variances)
  - ["Distributions" and "histogram data" are distinguished by their unit](#details:distribution-histogram-distinguised-by-unit)
  - [No progress reporting an no cancellation](#details:no-progress-reporting-no-cancellation)
  - [No support for histograms with varying length](#details:no-length-variation)
  - [Operations are threaded using OpenMP](#details:threading)
  - [No magic](#details:no-magic)
  - [No logging](#details:no-logging)
  - [Operations have strong exception guarantee or clear data if only a basic exception guarantee can be given](#details:exception-safety)
  - [No processing history, for the time being](#details:no-processing-history)
- [Implementation](#implementation)
  - [Milestones](#implementation:milestones)
  - [Effort](#implementation:effort)
- [Discussion](#discussion)
  - [Impact](#discussion:impact)


## <a name="context"></a>Context

This document describes a proposed new library, to become part of the Mantid project.
The working title of the project that led to the design proposed in the following has been **Workspace-2.0**, i.e., the set goal was to find a way to improve upon the workspace types that are currently available in Mantid.
For reasons that will become clear below we adopt the title `Dataset` instead of the old working title.

Before starting the design and prototyping phase, [comments about the current implementation and requirements for the new design](https://github.com/mantidproject/documents/blob/master/Project-Management/CoreTeam/workspace-notes.md) had been gathered.
For a big picture of the current situation, consider the class inheritance diagram for the existing workspaces in Mantid:

![figure: current inheritance diagram for workspaces in Mantid](workspace-hierarchy.png)

To put this diagram in perspective, note that one major point of criticism of the current implementation was "not enough workspace types", i.e., for the redesign an even larger variety was requested.
Based on these requirements a design and prototyping process was started in early 2018.
Initially a couple of different options have been prototyped but it soon became apparent that only one of them was a real option.
In agreement with reviewers of those early investigations, effort has thus been mostly focused on the `Dataset` option, described below.
Altogether, as of 2018-10-05, a total of 60 working days (approx 3.5 FTE months) have been invested in the design and prototyping process.


## <a name="overview"></a>High level design overview

We propose a new class called `Dataset`, with the capability to replace almost all existing workspaces types while at the same time providing more features and more flexibility.
The inspiration for `Dataset` comes from the `Dataset` type in [xarray](http://xarray.pydata.org/en/stable/).

Quoting from the website:
*"**xarray** (formerly **xray**) is an open source project and Python package that aims to bring the labeled data power of pandas to the physical sciences, by providing N-dimensional variants of the core pandas data structures."*

`xarray.Dataset` is roughly speaking a `dict` of `numpy.ndarray` objects with labeled coordinates and dimensions.
Our design adds features that are not present in `xarray.Dataset` but represent the core of what Mantid provides to neutron science: Handling histograms of (neutron) data, handling of physical units, and propagation of uncertainties.
Note that the proposed design is *not* a 1:1 copy of `xarray.Dataset`, there are some significant differences.
Furthermore, it will be written in C++ with Python exports for optimal performance and flexibility.

As described in this document, `Dataset` should be able to replace all of our workspace types, with maybe the exception of `MDEventWorkspace`.
This includes not just `MatrixWorkspace` and its child classes `Workspace2D` and `EventWorkspace` but also `TableWorkspace`, `PeaksWorkspace`, and `MDHistoWorkspace`.
In addition, `Dataset` can likely also replace `Histogram`, `EventList`, and data structure introduced as part of **Instrument-2.0**.
`Dataset` would provide a *uniform interface* in a *single type* for all of these.
Furthermore, `Dataset` will cover many other cases that are currently impossible to represent in a single workspace.


### <a name="overview:components"></a>Components

`class Dataset` is the top level entity.
It simply set of variables, instances `class Variable`.
`Variable` contains:
- Tag and name identifying the variable.
- Unit of the data held by the variable.
- Dimensions of the data held by the variable, i.e., the side lengths of a multi-dimensional cube of data, using `class Dimensions`.
- Data, stored in an array-like container.

The dimensions of the variables in a dataset can differ, but the extents of the individual dimensions must match.
*Example: A one-dimensional variable with `Dimension::X` and a two-dimensional variable with `Dimension::X` and `Dimension::Y` in the *same* dataset.
The respective extents of `Dimension::X` must match.*
A zero-dimensional variable corresponds to a point, i.e., it will contain a single data item.

Variables come in two classes, distinguished by their tag:
Coordinate variables and data variables.
This distinction enables a simple but effective definition of the behavior of `Dataset` in (arithmetic) operations:
- *Data variables* are *transformed*, e.g., subtracted in a "subtract" operation.
- *Coordinate variables* are *matched*.

As a simple example, consider subtracting two histogram:
We need to check that the X values match, and then subtract the Y values, i.e., the X values are coordinates, the Y values are data.

Examples of tags for data variables are:
- `Data::Value`
- `Data::Variance`
- `Data::String`
- `Data::Int`
- `Data::ExperimentLog`

Examples of tags coordinate variables are:
- `Coord::Tof`
- `Coord::dSpacing`
- `Coord::Q`
- `Coord::SpectrumNumber`
- `Coord::DetectorId`
- `Coord::SpectrumPosition`
- `Coord::DetectorPosition`
- `Coord::DetectorGrouping`
- `Coord::RowLabel`
- `Coord::X`, `Coord::Y`, `Coord::Z`
- `Coord::Qx`, `Coord::Qy`, `Coord::Qz`
- coordinates related to experiment environment parameters such as `Coord::Polarization` and `Coord::Temperature`
- `Coord::Mask`

The tag defines the underlying data type.

We furthermore define:
- Coordinate variables do not have a name and are thus identified only by their tag.
  Since `Dataset` is a set this implies that there cannot be two coordinate variable with the same tag in the same dataset.
  *Example: Having two variables `Coord::X` representing an X coordinate in a dataset is forbidden, and would indeed not make sense.*
- Coordinate variables have an implicit unit that cannot be changed (at least apart from its scale) since it is implied by the tag defining the coordinate.
  *Example: The X coordinate would always have a unit representing a *length*, e.g., `m` or `mm`.*
- Data variables have a name in addition to their tag.
  *Example: We can have a data variable holding neutron counts in `Data::Value` fom the "sample" run and a data variable holding counts for the "can" run in the *same* dataset.*

The name of data variables is furthermore used to imply a grouping of variables.
*Example: In a dataset containing data variables `Data::Value` and `Data::Variance` a matching variable name implies that the uncertainties stored in `Data::Variance` are associated with the data stored in `Data::Value`.*

The data in `Variable` is held by a type-erased handle, i.e., a `Variable` can hold data of any type.
The data handle also implements a copy-on-write mechanism, which makes `Variable` and `Dataset` cheap to copy and saves memory if only some variables in a dataset are modified.


### <a name="overview:operations"></a>Operations

Alongside the new `Dataset` type a series of basic and common operations will be provided.
A mostly but not entirely complete list of operations to be supported is:
1. Arithmetic operations: unary `-`, binary `+`, `-`, `*`, `/`
1. `transpose`
1. `slice` and `dice`
1. `concatenate` and `merge`
1. `sort`
1. `rebin`
1. `convert` (unit and/or dimension)
1. `filter`
1. Boolean/logical operations for mask operations
1. `integrate`
1. Basic statistics operations such as min, max, mean, and standard deviation


### <a name="overview:relation-to-existing-workspace-types"></a>Relation to existing workspace types

From the above description it may not be clear how `Dataset` is used to replace existing workspace types or components so we take a break to give some examples.
For clarity, in the following we are explicitly giving the current Mantid namespace names such as `API`, `DataObjects`, `Geometry`, and `Algorithms` when referring to types from the existing implementation.

##### DataObjects::TableWorkspace

A dataset where all variables are one-dimensional is a table.
- Each variable is a column.
- The variable name is the column label.
- `Coord::RowLabel` can provide a row label.
- `concatenate` is used to append one table to another.
- `merge` combines columns from two tables into one.
- `slice`, `dice`, or `filter` extract a new table with one or more rows.
- `sort` sorts the table.
- Coordinate and name matching enables mathematical operations on individual table columns.

##### DataObjects::MDHistoWorkspace

A dataset with multi-dimensional variables `Data::Value` and `Data::Variance` in combination with a corresponding number of one--dimensional coordinate variables is a close equivalent to the current `DataObjects::MDHistoWorkspace`.
A typical example would be three-dimensional `Data::Value` and `Data::Variance` alongside one-dimensional `Coord:Qx`, `Coord:Qy`, and `Coord::Qz`.

##### DataObjects::Workspace2D

A dataset with two-dimensional variables `Data::Value` and `Data::Variance` in combination with a one- or two-dimensional variable `Coord::Tof` is a close equivalent to the current `DataObjects::Workspace2D`.
- Typically `Data::Value` and `Data::Variance` would have dimensions `Dimension::Tof` and `Dimension::Spectrum`.
- If `Coord::Tof` has only `Dimension::Tof`, it corresponds to a `DataObjects::Workspace2D` with shared X axis, if it also has `Dimension::Spectrum` X is not shared.
- To represent bin edges, the extent of `Coord::Tof` in `Dimension::Tof` is larger by one than `Dimension::Tof` of the other variables.
- `Coord::SpectrumNumber` provides a spectrum number (currently in `API::ISpectrum`).
- Mapping from spectra to detectors (currently in `API::ISpectrum`) can be added using `Coord::DetectorGrouping`.
  For representing the instrument see the corresponding section below.
- Data from `API::ExperimentInfo` such as `API::Run` can be held by zero-dimensional variables such as `Data::ExperimentLog`.
- Masking of complete spectra or individual bins support by one- or two-dimensional variables such as `Coord::Mask`.

Most basic operations have an obvious mapping to what our current basic algorithms are doing.

##### Geometry::DetectorInfo, Geometry::ComponentInfo, and Geometry::Instrument

`Geometry::DetectorInfo` and `Geometry::ComponentInfo` are the result of the refactoring effort as part of **Instrument-2.0**.
Thanks to that refactoring, the underlying data is already stored in a structure of vectors, which can be mapped to variables in a dataset in an almost 1:1 manner.
- In place of `Geometry::DetectorInfo`, several variables with `Dimension::Detector` would be added to a dataset.
  - `Coord::DetectorPosition` to store positions, `Coord::DetectorRotation` to store rotations, ...
- The option to have multi-dimensional variables will provide support for instruments with (synchronously) scanning detectors in a natural way.
  - Variables such as `Coord::DetectorPosition` can have the additional `Dimension::DetectorScan` (in addition to `Dimension::Detector`).
  - Time intervals for the scan can be stored in an additional variable `Coord::TimeInterval`.
- Data for `Geometry::ComponentInfo` can be mapped to variables in the dataset in a similar manner.
- `Geometry::Instrument` still exists as part of the pre-**Instrument-2.0** legacy.
  For the time being it can be supported by simply holding it in a zero-dimensional variable.

##### HistogramData::Histogram

`Dataset` will store histogram data as a single multi-dimensional array, i.e., *not* as a variable holding histograms.
However, we may still want to operate on a single histogram at a time in some cases.
A dataset with one-dimensional data variables `Data::Value` and `Data::Variance` alongside a one-dimensional coordinate variable such as `Coord::Tof` is a histogram.
- Arithmetic operations as well as many of the other basic operations support most of what would be required from a histogram type.

##### DataObjects::EventWorkspace

In contrast to histograms in `DataObjects::Workspace2D`, the event lists stored in `DataObject::EventWorkspace` have varying length.
Therefore, the events cannot be stored as a single multi-dimensional array.
- A one-dimensional variable `Data::Events` with `Dimension::Spectrum` holds the lists of the events.
- The lists of events is a type similar to `DataObjects::EventList`.
  One option is to use `Dataset` for this, that is the dataset would contain many datasets, one for each event list.

##### DataObjects::EventList

A one-dimensional dataset without coordinate variables can be used to represent an event list.
- `Dimension::Event` is used as the only dimension for all variables.
- `Data::Tof` and `Data::PulseTime` make up the key part of data.
- Extra variables can be added to support weighted events.
- `concatenate` provides a merge operation.
- `sort` provides sorting by time-of-flight or pulse-time.
- `filter` provides filtering by time-of-flight or pulse-time.


### <a name="overview:python-exports"></a>Python exports

All relevant high-level classes and functions are fully exposed via a Python interface.
- The C++ API is modern and is thus easily mapped to a "pythonic" dataset library.
- `Dataset` itself is very similar to Python `dict`.
- Data of variables containing plain old data such as floating point values is exposed as readable and writable `numpy.ndarray`.
  These are true multi-dimensional arrays, i.e., in contrast to the current `API::MatrixWorkspace::readX()` and `API::MatrixWorkspace::dataX()` it is *not* a separate array for every spectrum.


## <a name="details"></a>Design details


### <a name="details:dependencies"></a>Dependencies

- `boost::units` for unit handling.
- `boost::container` for `small_vector`, which is used as an optimization in a couple of places.
- `boost::mpl` to help with some meta programming.
- `gsl` is used for `gsl::index` and `gsl::span`.
  This could easily be replaced by a stand alone implementation, in particular since `span` is part of C++20.
- `range-v3` has been used for implementing some multi-column sort operations but replacing it by a stand-alone implementation should be no trouble.
- GoogleTest for unit testing.
- GoogleBenchmark for benchmarking.

The prototype has been implemented using C++14, and this is sufficient.
Nevertheless, having C++17 available would be an advantage, particularly since a moderate amount of template-meta programming is required.
Given the long time scale, it could be expected that the Mantid project is ready to move to C++17 by the time `Dataset` might be included in the main repository.

Note that `Dataset` or rather `Variable` will be able to hold data from libraries that are *not* dependencies of this library.
It is sufficient to forward-declare types when adding a tag for such a variable.
*Example:
We can store `API::Run` in `Dataset`, but the dataset library does not need to depend on `Mantid::API`.*

It is currently unclear whether this library should be kept as a separate repository or be included as a new module in the Mantid repository.
- The advantage of includes is the automatic use of the build server and deployment infrastructure.


### <a name="details:unit-tests"></a>Unit tests

The library is intended as a low-level building block and the mixed-multi-dimensional aspected lead to many corner cases.
A comprehensive and maintainable suite of unit tests is thus essential.


### <a name="details:benchmarks"></a>Benchmarks

Given the intention of being a low-level building block for everything else, ensuring good performance and preventing performance regression is thus essential.
A suite of benchmarks is to be maintained and monitored for changes.


### <a name="details:documentation"></a>Documentation

Documentation is required for several cases:
- Library (developer) documentation.
- C++ API (user) documentation.
- Python API (user) documentation.
- Python usage examples beyond simple API documentation, e.g., how to represent complex data structures such as a `DataObjects::Workspace2D` as a `Dataset`.


### <a name="details:scope-and-extension"></a>Scope and extension

The design described here is seen as a nearly complete description of the scope of the dataset library.
Extending the functionally should be avoided and should happen only after careful consideration.
In particular, anything that is *more* specific than already supported should not be made part of this library, but instead be placed in another library that depends on this library.
- Actually `convert` to convert units is pretty specific as it needs to connect instrument information to data.
  Is having it as part of this lowest-level library the right choice?

There are some cases which are likely to lead to additions to the dataset library:
- Initially we will not have covered all required cases with tags for `Dimension` and variable tags for `Coord` and `Data`.
  In particular, we need a new `Coord` for every unit that we support.
- As a consequence the unit handling will also need to be extended.


### <a name="details:naming"></a>Naming

#### Components of the library are part of namespace `dataset`

##### Rationale

Mantid currently uses upper-case namespace names.
However, this can lead to artificial and unintuitive namespace or class names.
Instead, namespaces like `dataset` should be lower-case, as opposed to types which are upper-case.
We then have, e.g., `dataset::Dataset`.
This also matches the Python convention where module names are typically lower-case, thus reducing the differences between the C++ and Python APIs.

#### The Python module name is `dataset`

##### Rationale

This is a corollary from the namespace name.

#### The top level namespace and module is `mantid`

##### Rationale

Currently we have `namespace Mantid` in C++ while the Python module is `mantid`.
Changing the namespace name would this improve consistency.
Furthermore, this would reduce naming clashes between existing and new code.


### <a name="details:copy-on-write-mechanism"></a>Copy-on-write mechanism

##### Context

The copy-on-write mechanism is hidden in the API and simply handled internally in `Variable`:
A copy-on-write pointer hold a `VariableConcept`.
To maintain sharing we only require code to be `const`-correct.
*TODO: Cannot use `const` in Python, need methods with different names that explicitly request mutable or immutable access.*

##### Rationale

The copy-on-write mechanism is something that has since long been adopted in Mantid, e.g., in `DataObjects::Workspace2D` for the histogram data and for `Geometry::DetectorInfo`.
However, for the interface of `API::MatrixWorkspace` (and in connection to that `HistogramData::Histogram`) this has led to an interface that is too complicated, with different API functions for read-only access, write-access, and access for sharing.


### <a name="details:uncertainties-are-stored-as-variances"></a>Uncertainties are stored as variances, not standard deviations

##### Rationale

Mantid currently stores uncertainties as standard deviation (except for `DataObjects::MDHistoWorkspace`).
However, most computations require the variance for propagation of uncertainties.
To this end it is often squared in place (breaking assumptions about, e.g., `HistogramData::Histogram` and `DataObjects::Workspace2D`, and subsequently converted back to standard deviation by taking the square root.
This also adds a little computational overhead.
Having a clear and well defined strategy will reduce the risk of confusion.

##### Implementation

The go-to tag for storing uncertainties will be `Data::Variance`.
`Data::StdDev` will exist, but is for *access* only, i.e., attempting to create a `Variable` with that tag should fail.
Instead `Data::StdDev` can by used in combination with `DatasetView` to *read* the standard deviations if required, e.g., for visualization purposes.
If it turns out to be useful it would also be possible to provide setters --- as opposed to a simple assignment which is possible if the actual underlying data is referenced --- for standard deviation using `DatasetView` .


### <a name="details:distribution-histogram-distinguised-by-unit"></a>"Distributions" and "histogram data" are distinguished by their unit

##### Rationale

Currently `API::MatrixWorkspace` (and by extension `HistogramData::Histogram`) keep a flag indicating whether data in a histogram has been divided by its bin width.
A considerable number of algorithms are checking this manually, branching to different code sections.
`HistogramData::Histogram` attempted to solve this by introducing getters like `Histogram::counts()` and `Histogram::frequencies()`.
However, it seems to be useful only in a rather limited number of places, whereas it adds a significant amount of complication to the API.
The flag adds complication and should thus be removed.

##### Implementation

This will require having a unit for "counts", i.e., we cannot simply use "dimensionless".
We need to establish whether it is possible to write clean client code based on the unit-driven mechanism.
Apart from that there should not be any major implementation difficulties.


### <a name="details:no-progress-reporting-no-cancellation"></a>No progress reporting an no cancellation

##### Rationale

- Most operations are simple and thus fast.
- Cancellation and progress reporting can be implemented in higher-level algorithms that *use* `Dataset`.
- Cancellation leads to undefined side effects for in-place operation, i.e., it breaks data.
- Adds complexity and complicates the interface.
- `numpy` operations cannot be cancelled and do not report progress.


### <a name="details:no-length-variation"></a>No support for histograms with varying length

##### Rationale

This has not been supported in `DataObjects::Workspace2D` for 10 years, until a very recent change, i.e., it does not appear to be strictly necessary.
Support has been considered and prototyped, however there are two strong reasons it is not part of the final design:
- Significant complication of implementation for all operations, leading to larger initial development cost and more difficult long-term maintenance.
- Negatively affects the performance of iteration via `DatasetView`, *even if the length does not vary*.

##### Workaround

It is possible to store a variable holding datasets in a dataset.
The result could be very similar to a current `DataObjects::Workspace2D`.
In general we should discourage doing this since most of the other functionality we provide would not work nicely with such as nested dataset.
However, if a varying histogram length is essential for some purpose it is absolutely possible.


### <a name="details:threading"></a>Operations are threaded using OpenMP

##### Rationale

Threading using OpenMP in a fork-join style manner is not the most efficient one.
As part of the prototyping process we considered other solutions, in particular [dask](https://dask.org/).
Dask could solve many problems such as overcoming the limitations of the fork-join approach as providing a unified solution for threading and multi-process parallelisation such as MPI.
However, the prototyping experience indicated that it is not performant enough to completely replace threading for our typical workloads.
For that reason, and for maximum simplicity, optimal performance requires multi-threading.

##### Implementation

In most cases a simple `#pragma omp parallel for` could do the job.
We should probably limit the thread count depending on the size of data and the operation in question.
For small data the threading overhead may be larger than the gains.
For large data, many cases such as `operator+` are heavily limited by memory bandwidth and using more cores than necessary for reaching the peak bandwidth would be wasteful.


### <a name="details:no-magic"></a>No magic

##### Rationale

Many algorithms in Mantid to "magic" under the hood:
- Convert inputs if required. *Example: Conversion from point data in `Algorithms::ConvertUnits` and `Algorithms::Rebin`.*
- Tolerate partially bad input data. *Example: Replacing special values in `Algorithms::SumSpectra`.*
- Support doing two different things that are actually different. *Example: `Algorithms::Rebin` can rebin histogram data, bin event data, or set new bin edges in a workspace.*
- Combine multiple processing steps. *Examples: Aligning bins in `Algorithms::ConvertUnits`. Loading and saving calibration files in `Algorithms::MaskDetectorsIf`.*

Support for such features complicates the implementation and makes reasoning about code harder.
It should therefore not be done in this library.
- Operations should simply fail as early as possible.
- Inputs are required to be cleaned up and converted to the correct format explicitly.
- Processing steps should be kept as separate operations.

##### Note

There is nothing wrong with "magic" in higher-level operations.
High level algorithms that simply work no matter what is thrown at them are likely still a requirement.
We are merely banning it from *this* library, i.e., the lowest-level building block.


### <a name="details:no-logging"></a>No logging

##### Rationale

As a corollary from the "no magic" strategy above, there will be very few occasions that require log messages.
Logging can be implemented in higher-level algorithms, or potentially in some Python wrappers.
Adding logging functionality would increase complexity and add extra dependencies.

##### Workaround

If required we could log to `stdout` using `std::cout`, which can be redirected and filtered.


### <a name="details:exception-safety"></a>Operations have strong exception guarantee or clear data if only a basic exception guarantee can be given

##### Rationale

For performance reasons operations can be (and are in practice) applied in-place.
If an exception is thrown it can happen that data is partially modified and thus in an unknown state.
It is impossible for client code to tell if data has been corrupted, unless there is a well defined exception safety guarantee in combination with flagging of bad data.
The user thus continue working with the bad data, with unknown consequences.

##### Implementation

Wherever possible, operations should provide a strong exception guarantee, i.e., even for in-place operations there should be no risk of unknowingly corrupting data.
This simply boils down to performing any checks up-front, before data is modified.
For operations that cannot give a strong guarantee we need to avoid accidental use of corrupted data.
The safest and simplest way to do so is to completely clear datasets that may have been corrupted.

The exception safety guarantee of each operation is to be documented.


### <a name="details:no-processing-history"></a>No processing history, for the time being

##### Rationale

All Mantid algorithms are recorded in the workspace processing history, and this is one of the most important features of Mantid.
It is currently not clear how this is best handled for `Dataset`.
If the operations for datasets end up getting wrapped in algorithms there is no need to direct history handling in `Dataset`.
If this is not the case it would be very desirable to have this feature, and according to very basic prototyping it could simply be handled as an additional variable in the dataset.
There would be some complications related to having a history without `API::AnalysisDataService` so there is some effort connected to coming up with an actual solution.
Overall however, it should have no influence on the `Dataset` design, i.e., it seems safe to postpone a decision until we have a clearer understanding of how `Dataset` will be used in practice and how it will get integrated in the current Mantid ecosystem.


### Type erasure

The type erasure and implementation of basic operations for `Variable` uses **concept-based polymorphism** (a pattern by Sean Parent).
The involved types are `class VariableConcept` (the interface) and `template <class T> class VariableModel` (the implementation(s)).


## <a name="implementation"></a>Implementation

- efforts estimates (10x prototyping time and detailed list)
  - documentation (developer and user)
  - testing
- do not give estimates for things that we do not know about, e.g., time for implementing higher level algorithms --- we do not know if this is what people will want to do, how it works, ...



Implementation effort (impl, tests, doc):

early user (developer) testing, to continuously ensure we are implementing something that is actually useful
Dataset
Variable
Dimensions
DatasetView
helper classes
operations
"apply ufunc"?
Python exports (numpy adds difficulties, in particular since it can be enabled only for some types)
serialization and deserialization
continuous integration, build system, ...
ramp up time for every additional developer

slice viewer
integration in Mantid
  - integrate in build, such that you can import it in Python
  - wrap in Workspace (or not! given problems of ADS and Python! significant effort here to find a solution)
  - table viewer
converters from Mantid workspaces to Dataset (vice versa would be extra effort since there is no 1:1 mapping)

replace less-used workspaces types?
- MaskWorkspace?
- GroupWorkspace?
- MDHistoWorkspace?
- TableWorkspace?


### <a name="implementation:milestones"></a>Milestones
### <a name="implementation:effort"></a>Effort


## <a name="discussion"></a>Discussion
- user stories?
  - polarization
  - multi dimensional slicing
  - parameter scans
  - reactor constant-wavelength
  - imaging


- new challenges:
- zoo of datasets, standardize ways of representing things
- type erasure cumbersome?
- guarantees same API across types
### <a name="discussion:impact"></a>Impact



---

### Sharing of bin edges
- bin edges



- (single) precision
- alignment?

explain tradeof/cost of type-erasure (being generic, vs. some code noise)

enhancement list for each workspace example? or separate?
    Can in principle be multi dimensional.
    Can contain arbitrary types, so we could have an integer histogram, a histogram without uncertainties, a histogram with attached masking, ...
    concatenate
    Y and E can have units.
    X unit also stored in histogram.
    Can have a name.



`DatasetView`.
`DatasetIndex`.
`Dimensions`.

Dataset ser/deser
