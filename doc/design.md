# Dataset design

Author: Simon Heybrock

Prototype reviewers: Owen Arnold, Martyn Gigg

## Context

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

## High level design overview

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

### Components

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

### Operations

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

### Relation to existing workspace types

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
- `Coord::SpectrumNumber` provides a spectrum number (currently in `API::ISpectrum`).
- Mapping from spectra to detectors (currently in `API::ISpectrum`) can be added using `Coord::DetectorGrouping`.
  For representing the instrument see the corresponding section below.
- Data from `API::ExperimentInfo` such as `API::Run` can be held by zero-dimensional variables such as `Data::ExperimentLog`.

Regarding operations, most basic operations have an obvious mapping to what our current algorithms are doing.

##### Geometry::DetectorInfo, Geometry::ComponentInfo, Geometry::Instrument

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
A histogram would simply be represented by a dataset with one-dimensional data variables `Data::Value` and `Data::Variance` alongside a one-dimensional coordinate variable such as `Coord::Tof`.
- Arithmetic operations as well as many of the other basic operations support most of what would be required from a histogram type.

##### DataObjects::EventWorkspace

In contrast to histograms in `DataObjects::Workspace2D`, the event lists stored in `DataObject::EventWorkspace` all have varying length.
Therefore, the events cannot be stored as a single multi-dimensional array.
- A one-dimensional variable `Data::Events` with `Dimension::Spectrum` would hold the lists of the events.
- The lists of events would be a type similar to `DataObjects::EventList`, and one option is to use `Dataset` for this, that is the dataset would contain a list of datasets, one for each event list.

##### DataObjects::EventList

A one-dimensional dataset without coordinate variables can be used to represent an event list.
- `Dimension::Event` is used as the only dimension for all variables.
- `Data::Tof` and `Data::PulseTime` make up the key part of data.
- Extra variables can be added to support weighted events.


TODO make enhancement list for each? or separate?

Enhancements:
- Can have more dimensions.
- Can have multiple data.




    Can in principle be multi dimensional.
    Can contain arbitrary types, so we could have an integer histogram, a histogram without uncertainties, a histogram with attached masking, ...
    concatenate
    Y and E can have units.
    X unit also stored in histogram.
    Can have a name.





Type-erased handle to the data.

`DatasetView`.
`DatasetIndex`.

- explain how current types map onto `Dataset`

### Key concepts

- history
- logging
- cancellation
- bin edges

### Python exports

POD-Variables exposed as read/write `numpy.ndarray` with correct dimensionality (not just 1D arrays as current `MatrixWorkspace::readX()` and `MatrixWorkspace::dataX()`.
copy-on-write (explain hidden)

## Goals and non-goals

## Milestones

 - user stories?

## Impact

## Discussion






- numpy read/write wrappers, as old readX/dataX, but now multi-dimensional!

## Details

explain tradeof/cost of type-erasure (being generic, vs. some code noise)

C++14, maybe even C++17?
Dependencies: range-v3, gsl, boost, but in most cases very little, could make it almost completely stand-alone (with exception of boost)?

## Naming

### Components of the library are part of namespace `dataset`

##### Rationale

Mantid currently uses upper-case namespace names.
However, this can lead to artificial and unintuitive namespace or class names.
Instead, namespaces like `dataset` should be lower-case, as opposed to types which are upper-case.
We then have, e.g., `dataset::Dataset`.
This also matches the Python convention where module names are typically lower-case, thus reducing the differences between the C++ and Python APIs.

##

### Uncertainties are stored as variances, not standard deviations

##### Rationale

* [R: test](#Rr-scoped)

### <a name="Rr-scoped"></a>R.5: 



- efforts estimates (10x prototyping time and detailed list)
  - documentation (developer and user)
  - testing
- rollout
  - Python ADS issues
- do not give estimates for things that we do not know about, e.g., time for implementing higher level algorithms --- we do not know if this is what people will want to do, how it works, ...
- guarantees about not breaking inputs on failed operations (exception safety) --- explicitly specify in each case! clear if broken

- many parts of Instrument-2.0 can be mapped nearly 1:1
- scanning
- within Mantid repo or separate?
- units

- library structure, keep it small and do not add more random things over time, build other things on top of this lib.
