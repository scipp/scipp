Adding workspace types that can hold and provide access to a wide variety of data is rather simple.
In contrast, the key problems to address:
- How do we aggregate data with its meta data (meta data in the most general sense):
  - instrument
  - mapping from spectra to detectors in instrument
  - masking / region-of-interest
  - experiment logs
  - sample
  - axes
  - view into data
  - links to related workspaces
  - history
  - tags
- How can our algorithms support a wider range of workspaces, without adding distinct algorithms for various workspace types?
  - creating workspaces must be simple
  - making code into an algorithm should be simple, at least if the algorithm works only on individual spectra, or individual meta data categories 
  - access via properties and dynamic casting to concrete types may be too cumbersome if more workspaces types are added
- Overarching design principle should be an intuitive and simple way of using workspaces and writing algorithms.

Complaints that writing algorithms is too hard (also by power users?). Workspace types are responsible for that, in large parts.

# Definitions

For brevity and clarity in the discussion to follow we define the following dummy classes, used when referring to *new* workspace types:
```c++
struct DataPoint {
  double value;
  double error;
};

struct Histogram; // same or similar to current implementation
struct EventList; // same or similar to current implementation

template <class T>
struct Workspace {
  std::vector<T> data;
};

// Workspace<DataPoint> ~ Workspace2D with size 1, as used at reactors
// Workspace<Histogram> ~ Workspace2D
// Workspace<EventList> ~ EventWorkspace
```

### Duplicate concepts

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

There are often slight differences between the diffierent concepts, e.g., (1) masking of bins and masking of spectra/detectors is not the same, or (2) grouping of spectra is not the same as grouping detectors, however we can likely find a common way of expressing these concepts.
Eliminating duplication has several advantages:
- Less code duplication.
- Less confusion between different representations of the same concept.
- Less risk of algorithms breaking workspaces by incorrect processing and or propagation of the different instances of the same concept.

The above concepts could be unified in a way similar to the following:
- Replace masking mechanisms in the workspace/instrument by linking a workspace to a `MaskWorkspace`.
- Replace grouping mechanisms in the workspace by linking a workspace to a `GroupingWorkspace`.
- Remove the X-axis (keep only the unit).
- Remove the Y-axis (keep only the unit) and generalize the `SpectrumNumber` to something that can deal with more general data that might be used to label the Y-axis, such as a two-theta value.

Note that while this sounds simple there are a couple of subtleties for each of those items.

### Key question: How do we handle a larger variety of workspaces in our algorithms?

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

From the shortcomings listed above we can may conclude that providing a common workspace API is not feasible, or that the feasible level of common API is generally not benefical.

If we were to change this a new new design we need to solve two problems:

1. Algorithms should work for a variety of data types. For example, we need to be able to add `Workspace<EventList>`, `Workspace<Histogram>`, and `Workspace<DataPoints>`.
2. The ADS holds workspaces via some sort of type-erased handle.
  Algorithms need the actual type unless they use only API that is common to all workspace types held by the handle.

Solutions:

1. Define operations/operators for items in workspaces, e.g., `DataPoint::operator+()`, `Histogram::operator+()`, and `EventList::operator+()`.
  The part of algorithms dealing with transforming the actual data could then be templated to deal with the various workspace and item types.
  This way of writing algorithms would also enforce a better structure and modularity in our algorithm code.
2. There are several options:
   1. Pass type-erased workspace handle to algorithm, e.g., from `getProperty()`, cast to actual type in algorithm, branch into overloaded or templated algorithm components.
   2. Cast to actual type in algorithm wrapper, pass workspace of actual type to overloaded or templated algorithm entry point.
   3. Make the ADS Python-only, `pybind11` or similar will deal with "casting" and overload resolution automatically.

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
