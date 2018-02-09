# Initial thoughts an a new workspace design

## Overview

Adding workspace types that can hold and provide access to a wide variety of data is rather simple.
In contrast, the key problems to address are:
1. How do we aggregate data with its meta data (meta data in the most general sense):
   - instrument
   - mapping from spectra to detectors in instrument
   - masking / region-of-interest
   - x-uncertainty (SANS special case currently held in `Histogram`)
   - experiment logs
   - sample
   - axes
   - view into data
   - links to related workspaces
   - history
   - tags
2. How can we simplify writing algorithms and minimize the addition of distinct algorithms for an increasing number of workspace types?
   - A guiding design principle must be ease of use and intuitive ways of writing algorithms.
   - Creating and maintaining workspaces must be simple.
     The fact that a workspace is an aggregate of a fair number of components must not interfere with client code doing simple modifications of individual components.
   - Transforming code into an algorithm should be simple, at least if the algorithm works only on individual spectra, or individual meta-data categories.
   - Access via properties and dynamic casting to concrete types may be too cumbersome if more workspaces types are added.

## Definitions

For brevity and clarity in the discussion to follow we define the following dummy classes, used when referring to *new* workspace types:
```c++
struct DataPoint; // a single value with uncertainty
struct Histogram; // same or similar to current implementation
struct EventList; // same or similar to current implementation

template <class T>
struct Workspace {
  std::vector<T> data;
};

// Workspace<DataPoint> ~ Workspace2D with x-size 1, as used at reactors
// Workspace<Histogram> ~ Workspace2D
// Workspace<EventList> ~ EventWorkspace
```

## Duplicate concepts

Considered from a general point of view, it turns out that there is a fair amount of duplicate concepts hidden within our workspace types:

1. Masking:
   - `MaskWorkspace`.
   - Masking of detectors in the instrument.
   - Masking of individual bins for individual spectra.
2. Grouping:
   - `GroupingWorkspace`.
   - Grouping via a set of detector IDs in `ISpectrum`.
   - Grouping via `SpectrumDefinition` in `IndexInfo`/`SpectrumInfo` (this is was introduced as duplicate mechanism of the grouping via `ISpectrum`, but generalized to support scanning instruments and MPI).
3. X-axis (values, excluding unit):
   - `MatrixWorkspace::x(i)`.
   - `MatrixWorkspace::getAxis(0)`.
4. Y-axis (values, excluding unit):
   - `MatrixWorkspace::getSpectrum(i).getSpectrumNo()` (also duplicated in `IndexInfo`).
   - `MatrixWorkspace::getAxis(1)`.

There are often slight differences between the different concepts, e.g., (1) masking of bins and masking of spectra/detectors is not the same, or (2) grouping of spectra is not the same as grouping detectors, however we can likely find a common way of expressing these concepts.
Eliminating duplication has several advantages:
- Less code duplication.
- Less confusion between different representations of the same concept.
- Less risk of algorithms breaking workspaces by incorrect processing and or propagation of the different instances of the same concept.

The above concepts could be unified in a way similar to the following:
- Replace masking mechanisms in the workspace/instrument by linking a workspace to a `Workspace<bool>` (or rather a dedicated new `MaskWorkspace`).
- Replace grouping mechanisms in the workspace by linking a workspace to a `Workspace<SpectrumDefinition>` (or rather a dedicated new `GroupingWorkspace`).
- Remove the X-axis (keep only the unit).
- Remove the Y-axis (keep only the unit) and generalize the `SpectrumNumber` to something that can deal with more general data that might be used to label the Y-axis, such as a two-theta value.

Note that while this sounds simple there are a couple of subtleties attached to each of these items.

## Supporting a larger variety of workspaces in algorithms

Currently Mantid attempts to handle a range of workspaces types via polymorphism.
The key example is `MatrixWorkspace`, providing a common API for `Workspace2D`, `EventWorkspace`, and others.

#### Shortcomings

- While the API is common up to a point, many algorithms need to treat `Workspace2D` and `EventWorkspace` separately.
  This is a typical example where inheritance does not match the problem.
- If we add more workspace types, the common parts of the API will shrink and more algorithms will need to distinguish between more workspaces types.
  As a consequence the complexity of our algorithm code will increase.

Examples:
1. Both `Workspace2D` and `EventWorkspace` provide access to counts in histograms, but it is read-only in the latter.
2. Constant-wavelength instruments use `Workspace2D` to store a single count value per pixel, but nevertheless the counts are stored in a vector as for time-of-flight data.

#### Considerations

From the shortcomings listed above we may conclude that providing a common workspace API is not feasible, or that the feasible level of common API is generally not beneficial.

If we were to change this in a new design we need to solve two problems:

1. Algorithms should work for a variety of data types. For example, we need to be able provide `Add` for `Workspace<DataPoint>`, `Workspace<Histogram>`, and `Workspace<EventList>` and `Rebin` for the latter two workspace types.
2. The ADS holds workspaces via some sort of type-erased handle.
  Algorithms need the actual type unless they use only API that is common to all workspace types held by the handle.

Possible solutions:

1. Define operations/operators for items in workspaces, e.g., `DataPoint::operator+()`, `Histogram::operator+()`, and `EventList::operator+()`.
  The part of algorithms dealing with transforming the actual data could then be templated to deal with the various workspace and item types.
  This way of writing algorithms would also enforce a better structure and modularity in our algorithm code.
2. There are several options:
   1. Pass type-erased workspace handle to algorithm, e.g., from `getProperty()`, cast to actual type in algorithm, branch into overloaded or templated algorithm components.
   2. Cast to actual type in algorithm wrapper, pass workspace of actual type to overloaded or templated algorithm entry point.
   3. Make the ADS Python-only, `pybind11` or similar will deal with "casting" and overload resolution automatically.

Requiring different overloads/templates from algorithms supporting similar but distinct workspace types may seem like a big overhead in development effort.
I believe such an argument actually just highlights an underlying issue:
Our algorithms are generally too large and do too much.
Therefore providing overloads sounds like a big task.
Why do algorithms do too much?
If we take a closer look, the operation done by many algorithm is actually quite simple.
Complication is usually just added by dealing with special cases related to slightly different input, creating output workspaces in a correct and complete way, and sanitizing the output.
Thus the actual issue is a lack of lower-level modularity, and a substantial overhead of converting code into an algorithm.
Therefore, when considered on a short time scale, adding one big algorithm is quicker than adding three small algorithms (plus optionally a higher-level algorithm using the three smaller ones).

A frequent example of overhead effected by workspaces is actually the `Histogram` type, which, for historical reasons, is a chimera of four different things (combinations of `Histogram::XMode` and `Histogram::YMode`).
We should probably consider carefully whether this should be changed, yielding four different workspace types in place of a single `Workspace<Histogram>`.

### Iterators

Iterators are a widely requested and simple addition for workspaces.
It should be pointed out that the advantages go beyond the obvious:
We can hide the internal structure of `Workspace<T>` to a certain extent.
`Workspace<T>` is likely to contain several components that probably have a vector-like interface, such as the actual vector of data items, instrument access via `SpectrumInfo`, and access the meta-data for spectra such as spectrum numbers.
Similar to the recent addition of iterators for `Histogram` we can provide a wrapping item type for workspace that provides access to all of the indexable content of a workspace.
This would provide an interface similar to what `ISpectrum` provides, with the addition of instrument access.

Such an abstraction could also help to keep code generic in case we choose to encode more than just the item data type in the workspace type.
One example could by an alternative instrument representation if only L2 and 2-theta tables are available:

```cpp
Workspace<Histogram, SpectrumInfo>;
Workspace<Histogram, L22ThetaTable>;
```

We could then implement `Rebin` based on an iterator that does not contain instrument access (since `Rebin` does not need it), keeping the code the same without the need for templating.
Algorithms that do need instrument access could use iterators with instrument access and would obviously need to be overloaded for the different instrument types.

### Algorithm-workspace interaction classification

To get a better understanding of the required operations on the set of data and meta-data components in an workspace, let us attempt to classify our existing algorithms by subset of components and the type of interaction with the components.
The goal is the get a better understanding of operations that workspaces must readily support, and to avoid blind spots during the design process.

To begin, let us note that all algorithms need to update the history, but that can be handled in the algorithm wrapper.
There are a couple of exceptions like `LoadNexusProcessed` and `SaveNexusProcessed`.

The following overview currently glosses over the creation of output workspaces, which must somehow deal with all components, i.e., the classification is based on the required input only.
In many cases creating outputs could probably be covered by in-place modification (potentially on a copy of the input, unless the user request in-place operation).

#### Categories

##### 1. Data (R)

Algorithms computing a result based on read-only data, without relation to the instrument or experiment.
Examples: `Fit`, `FFT`.

##### 2. Data (R+W)

Algorithms modifying data, without relation to the instrument or experiment.
Examples: `Rebin`, `Scale`, `BinaryOperation` (with the exception of operations between non-matching workspaces), `ConvertToHistogram`/`ConvertToPointData` (unless we handle this as different types), `CompressEvents`, `SortEvents`.

##### 3. Single metadata (R+W)

Algorithms modifying a single meta-data component.
Examples: `MoveInstrumentComponent`, log filtering/editing, masking (unless handled in the current way, which also clears data).

##### 4. Data (R+W) plus single metadata (R)

Algorithms that modify data based on metadata content.
Examples: `FilterByLogValue`, `RemovePromptPulse`

##### 5. Data (R+W) plus one or more metadata (R+W)

Examples: `ConvertUnits` (modifies data and unit based on instrument), `ExtractSpectra` (filters spectra, implies filtering of bin masking and implicitly the spectrum-detector mapping and spectrum numbers)

##### 6. Data (R) plus one or more metadata (R)

Examples: `Save` algorithms.

##### 7. Data (W) plus metadata (W)

Examples: `Load` algorithms.

##### 8. Complex interaction, potentially with multiple workspaces

Examples: `MergeRuns` and other stitching algorithms

#### Comments

- Categories 1.), 2.), and 3.) could mostly be handled by code that does not know about workspaces.
  For these, the less coupling there is between components in a workspace, the better.
  Encapsulation could be very good and algorithm code could very low-level, provided that we find ways to create output workspaces automatically in an algorithm wrapper.
- Category 4.) is like the previous ones, except that an extra read-only metadata input is required.
  Probably this can be handled in the same way.
- Category 5.) feels like the key complexity problem we have in Mantid.
  Several components in a workspace are modified, while others need to be preserved or copied.
  Attempting to split such algorithms into several components that each modify only a single component could potentially help us here, i.e., we might be able to move some of these into category 2.), 3.), or 4.).<sup>1</sup>
- For categories 6.) and 7.) similar comments as for 5.) apply, but here as well we cannot solve all cases in a nice way.

Overall, we are mostly dealing with two types:
1. Interaction with single components (for all indices).
2. Interaction with single indices (for all components that are indexable)

This could imply that we need to support interface workspaces in two ways, as a *structure of arrays* as well as an *array of structures*.
It would be beneficial if this allowed for hiding the internal data layout of a workspace, since it is just an implementation detail.<sup>2</sup>
This concept could be further extended to support groups/stacks of workspaces with identical meta data (such as multi-period workspaces) within a single workspace.

<sup>1</sup> <small>For example, an algorithm like `ExtractSpectra` (without the bit that crops histograms) could be implemented as a generic operation on vectors of data or meta data.
If histograms, bin masking, spectrum-detector mapping, and spectrum numbers were each a separate entity in a workspace we could simply apply the `ExtractSpectra` "algorithm" to each of these individually.
It is not clear if it would be feasible to handle that automatically.
Another way would be to group data and metadata on an item level (similar to what `ISpectrum` provides, but with the addition of all other information like bin masking).
This might be supported by zipping data with metadata when iterating, e.g., as supported by `ranges-v3`.</small>

<sup>2</sup> <small>The same concept could be extended to the `Histogram` level, but making it work well could prove to be difficult.
Imaging would benefit from storing data as a stack of images (once for each TOF), but still have data accessible as histograms.
If X is shared by definition, a histogram in the current sense may not be the right implementation anyway.
A histogram of **images** might be a better way to think about the data, and an efficient implementation of `Rebin` and similar algorithms seems obvious (operations that break the joint binning such as certain unit conversions would imply a transformation into a workspace type containing a vector of histograms).
This would also get rid of the need to share X data on the level of individual histograms?
</small>

### Compatibility with existing algorithms and workspaces

Big parts of the discussion above considers workspaces types incompatible with current algorithms, as well as new algorithms that might be incompatible with current workspaces.
Specific solutions have to be postponed until a design draft is available, but generally speaking we have the following tools at our hand:
- Begin implementation with
  - existing workspaces types that are used infrequently and
  - new workspace types that replace current workarounds.
  These would not involve refactoring major fractions of our algorithms.
- Wrap new workspaces in old workspaces or, less likely, old workspaces in new workspaces.
- Provide converters between old and new workspaces, where applicable (not all new workspace types will have a corresponding old workspace type).
- Provide new algorithms mainly for new workspace types that have no support or workaround currently.
