# Scipp as a C++ library and the scientific Python software ecosystem

## Purpose

The purpose of this document is an updated look at the decision to implement Scipp in C++, the developments in the Python ecosystem in the past couple of years, and how they may relate to the future of Scipp.

## Background

### Origins

Scipp was born out of a design proposal and prototype for a possible replacement of Mantid's workspaces, a hierarchy of classes for storing all kinds of data.
The design started in January 2018, resulting in the "Workspace 2.0" proposal towards the end of 2018.
With hesitant feedback from neutron-scattering facilities other than ESS, 2019 marked a shift in focus to develop the prototype into the stand-alone Scipp library.
At the time, the goal was to use this in combination *with* Mantid but not *in* Mantid.
This was in the context of a projected ESS hot-commissioning date some time in 2022.

### Initial requirements and boundary conditions

The high-level requirements in 2019 could be summarized as follows:

1. C++ core with Python bindings.
   This originates from a combination of aspects:
   - We considered it likely that we would eventually interface with Mantid (or other applications) on the C++ side to ensure maximum performance.
   - We knew that multi-threading is crucial.
   - Most (maybe 80% to 90%) of our team's expertise was in C++.
     While there was basic Python knowledge throughout the team, we lacked experience in developing Python libraries and, most importantly, background knowledge in the Python ecosystem.
2. Support for labeled dimensions.
3. Support for physical units.
4. Support for propagation of uncertainties.
5. Support for histograms (bin-edge coordinates).
6. Support for masks.
7. Support for ragged data (event data).

Aside from these concrete requirements, the central part that Jupyter would play in our software strategy was not anticipated in 2019.
At the time we mostly envisioned use through Python scripts or specialized GUIs.

## Scipp status

Scipp implements all the requirements listed above.
Essentially all operations are multi-threaded using TBB.
Some aspects have evolved significantly.
For example, the support for ragged data (binned data) is simpler and more natural than the initial design in 2019.
The physical unit system is based on an external C++ library, [LLNL/units](https://github.com/LLNL/units).

On the C++ side, almost all high-level code for `DataArray` and `Dataset` is implemented with the type-agnostic `Variable`.
Lower level operations are mostly generated from a simple kernel, with a call to a generic `transform` function template.
That is, a lot of Scipp's C++ code looks quite similar to what one would write based Scipp's Python API.

Interoperability with the Python world is ensured by exposing arrays using the buffer protocol.
That is, Python functions that expect NumPy arrays can operate on these buffers.

A lot of additional functionality has been implemented as well.
It should be highlighted that all this is pure Python:

- Coordinate transformations (`sc.transform_coords`).
- Jupyter visualization features (such as `_repr_html_`, SVG, and table views).
- Plotting in any number of dimensions (currently being moved into `scipp/plopp`).
- Loading NeXus files (`scipp/scippnexus`)
- Basic HDF5 I/O.
- Wrappers for some SciPy functions.
- ESS-specific workflows (`scipp/ess`).

## Downsides

### General

At the time of writing (2022) Scipp seems to work really well, performance appears to be adequate, and we receive positive feedback from multiple parties.
Nevertheless, there are a number of problems and downsides, some of which we outline in the following.

- Non-neutron facilities such as synchrotron facilities or free-electron laser facilities produce considerably more data than neutron-scattering facilities.
  While we always received positive comments, the lack of Dask support seems to be one of the major obstacles for potential adoption.
- Lazy loading is a feature we get asked about repeatedly (even though it is unclear how relevant this is for neutron-scattering data itself).
  This is something Dask could support.
- Xarray is very similar to Scipp in a number of aspects.
  As it is much more widely used and supported, it is unlikely that Scipp can attract outside interest.
  This is especially true given the developments in Xarray and the surrounding ecosystem since 2019 and their relation to our high-level requirements.
  See below for a detailed discussion.
- Not wrapping NumPy arrays may require copies when users interface with other libraries.
  This is not an issue when, e.g., loading data with `h5py` since those copies can be avoided, but it may be so for other applications.
- We are not using and leveraging developments in the scientific Python ecosystem.
- We are not contributing to the scientific Python ecosystem, even though many of the things we work on are not neutron-scattering specific and are being pursued by other projects.

In the bigger picture, the final to items above may be the most crucial ones.
Scipp risks to turn out as a permanent in-house project (with hypothetical but possible adoption in a couple of neutron-scattering facilities in the future).
As such, while an improvement over many aspects of Mantid, at a long time scale it might suffer a similar fate:

- Tightly scoped support and adoption (both in-house) while the outside world moves on.
- Limited benefit derived from outside progress.
- Users and developers require specific training (albeit less than for Mantid).
- Risk of reaching a tipping point, at which a "big rewrite" is undertaken.

### Unresolved problems and other subtleties

There are a few unresolved issues in Scipp that are worth considering.
Some of these are quite specific and this subsection can safely be skipped.

- [#2813](https://github.com/scipp/scipp/issues/2813) can prevent garbage collection in rare cases.
  While likely not common in practice, this hints at potential much wider issues if we chose to change `scipp.Variable` to support owning NumPy buffers.
- [#2675](https://github.com/scipp/scipp/issues/2675) is the latest instance of periodically resurfacing considerations and discussions about "attributes" in Scipp.
  Scipp's attributes are essentially "unaligned coordinate", cause some subtleties in `scipp.Dataset`, and adding Xarray- or NeXus-style `attr` dictionaries causes a name clash.
- Hard-coded set of dtype combinations for all operation can causes surprises annoyance in practice.
- Our mask handling is actually slow in some cases since masks or applied by making a copy before, e.g., a `sum` or `mean` operation.
  Despite multi-threading we are thus barely faster single-threaded than `numpy.ma`.
  This is easy to resolve, but may serve as an example of extra work due to bespoke solutions.

## Outside world

A brief review of the "outside world" reveals that the high-level Scipp requirements are being pursued by other projects as well.
A lot of this has been (and still is) happening since 2018/2019, i.e., some of this is parallel development and was not available as such during the infancy of the Scipp project.

- The [Python array API standard](https://data-apis.org/array-api/latest/) saw its first (non-draft) release in December 2021.
  This *standardize[s] functionality that exists in most/all array libraries and either is commonly used or is needed for consistency/completeness*.
  The goal is to address the fragmentation between libraries such as NumPy, Tensorflow, PyTorch, Dask, JAX, CuPy, MXNet, Xarray, and others.
- NEP18 added `__array_function__` (in addition to the previous `__array_ufunc__`), experimental in numpy-1.16 (Jan 2019), default in numpy-1.17 (July 2019).
- Based on `__array_function__`, Xarray could work on better [Pint](https://github.com/hgrecco/pint) support, resulting also in the experimental [pint-xarray](https://github.com/xarray-contrib/pint-xarray).
- The above is part of the wider "duck array wrapping" effort, see the [ongoing Xarray project](https://github.com/pydata/xarray/projects/2) and the [Duck array compatibility meeting](https://github.com/pydata/xarray/issues/5648).
- The [Xarray roadmap](https://docs.xarray.dev/en/stable/roadmap.html):
  - [Extracting xarray.Variable into a standalone package](https://docs.xarray.dev/en/stable/roadmap.html#labeled-array-without-coordinates) is a highly relevant development.
    It would allow for use of this component without the much larger Xarray (and Pandas) dependency.
    More details can be found in the related [proposal from September 2021](https://zenodo.org/record/5484176).
  - `xarray.DataTree` on the roadmap is relevant for us, e.g., for representing data from NeXus files.
    This goes beyond anything we have planned for Scipp, but tree like structures are encountered repeatedly, i.e., in neutron data reduction workflows, but have so far always been address by crude flattening of the structures, or be used of nested Python dicts.
- [Awkward array](https://github.com/scikit-hep/awkward) is a library for nested, variable-sized data.
  It was first released in 2018, reaching version 1.0 in December 2020.
  It is currently undergoing a rewrite, moving a lot of code to Python for a 2.0 release.
  It supports data structures similar to Scipp's *binned data* (possible limitations: not in-place, not multi-threaded except for the experimental [dask-awkward](https://github.com/ContinuumIO/dask-awkward)).
  There is an ongoing discussion for an [Awkward array backend](https://github.com/pydata/xarray/issues/4285) in Xarray, again based on what is possible thanks to `__array_function__` and the work going into duck array wrapping.
- Xarray is pursuing [flexible indexes](https://docs.xarray.dev/en/stable/roadmap.html#flexible-indexes) (see also the [Explicit Indexes project board](https://github.com/pydata/xarray/projects/1)).
  This may serve (among other purposes) Scipp's distinction between `coords` (aligned coordinates) and `attrs` (unaligned coordinates, not to be confused with Xarray `attrs`).
  Better support for custom indexes may also be relevant for features such as bin edges.
  See also the item on Pandas `IntervalIndex` in the next section.
- [NEP41]( https://numpy.org/neps/nep-0041-improved-dtype-support.html) aims for generalizing the NumPY dtype system, allowing for, e.g., handling physical units as part of the dtype.
  This is a long way from complete but highlights the direction the ecosystem is moving in.

## Requirements revisited

Aside from the developments in the outside world, which would support some of our high-level requirements, some of the high-level requirements from 2018/2019 have evolved:

- We do not foresee Scipp being used as a C++ library.
  Even if some internals are implemented in C++, other projects and communities have proven that interfaces between libraries and application via Python are feasible.
- Our team's expertise has evolved, gaining more Python experience and background knowledge across the board.
  We have the opportunity to strengthen this further with the upcoming recruitments.
- Possibly less emphasis on the prevailing (in the neutron scattering data reduction world) method of propagation of uncertainties.
  New approaches may need to be explored, and more rigid C++ libraries and data structures may make this more challenging.
- Develop something with wider support and adoption, within the neutron scattering community but also beyond.
  While this may sound odd as a requirement, it should be seen in the context of "requiring" a software solution that is maintainable on long time scales (more than 10 years) and comes with the ability to change things.

## Hypothetical future of Scipp

Given the developments outlined above, we may consider if and how it might be possible to replace Scipp or parts of Scipp using other Python libraries, and determine which piece, if any, are still missing or lacking.

The central feature we would need to rely on NumPy's duck array and the `__array_ufunc__` and `__array_function__` protocols.
In the following we will refer to this simply as *duck array*.
We would need the following:

- Duck array with labeled dimensions.
  This is on the Xarray roadmap, moving `xarray.Variable` into a standalone package.
- Duck array with physical units.
  `Pint` appears to provide this by now.
- Duck array with uncertainties.
  The Python `uncertainties` packages provides this, but it is likely not usable since it tracks correlations and is not computationally feasible for our purposes.
  We would thus need to provide a simpler duck-array error-propagation package.
  Early on in the Scipp development we argued that uncertainty propagation based on NumPy array math may be 10x slower (for in-place `multiply` or `divide` operations) and we therefore need to implement this in C++.
  There are two relevant notes here:
  - This may still be partially true, but this is a statement about the implementation of the *operation*, not the *data structure*.
    That is, we could still provide a Python-only duck array for this.
  - Multiplication and division operations with uncertainties and broadcast that commonly arose in neutron-scattering data reduction will be removed from Scipp due to the unhandled introduction of correlations.
    The most performance relevant aspect may thus vanish.
    See upcoming publication.
- Bin-edge coordinates are not supported in Xarray.
  However, Pandas' `IntervalIndex` and `Interval`, first introduced in pandas-0.20 (May 2017) and modified through 2019, partially cover our needs for bin-edges.
  It does not support math operations, i.e., is not directly suitable for coordinate transformations.
  This requires more investigation.
  One possibility may be a bin-edge duck array, optionally wrapping `IndervalIndex`.
- Scipp's masking feature supports a `dict` of masks, in contrast to `numpy.ma` which provides a single mask.
  Scipp applies masks selectively in reduction operations.
  This seems to be a simple case where we could provide a "multi mask" duck array handling the mask dict, forwarding, e.g., to `numpy.ma` for operations.
- Scipp's binned data could be represented as a duck array wrapping `awkward.array`.
  As more parties are interested in Xarray support there may be common interest in a joint development (inside or on top of `awkward`).
  Binned data is frequently used with a content buffer given by a `DataArray` or `Dataset`.
  Awkward supports custom records and provides a mechanism for customizing behavior of operations, but it is not clear if this is sufficient, i.e., it is not clear if an Awkward-based solution is actually feasible.

All of the above are essentially independent of each other, with exception of the binned-data support.
That is, each of the items above may be supported as a standalone Python package.
Most of these may be carried by a large community, i.e., this part of Scipp's scope is no longer tied to Scipp itself.
The duck arrays could internally use `numpy`, but would equally well work with `dask.array`.

Aside from this "stack" of duck arrays, the remaining Scipp scope comprises:

- Binning, grouping, histogramming, and resampling operations.
  These would need to be provided as a library that can operate on "arbitrary" duck arrays.
- `scipp.DataArray` and `scipp.Dataset`.
  One option would be to simply use Xarray.
  API differences could either be rolled out to our codebase, or wrappers would be provided.
  A second option would be a Python-only `scipp.DataArray` reimplementation, built on the stack of duck arrays outlined above.
- Multi-threading is crucial for the single-node performance in neutron scattering data reduction workflows.
  NumPy does not multi-thread BLAS vector-vector operations so other solutions would need to be investigated.
  One option could be the existing Scipp framework for operations on `scipp.Variable`, adopted to work on any NumPy-like buffer.
  This could be another duck-array, wrapping an underlying array implementation but providing multi-threaded operations (if implemented, otherwise fallback to calling underlying array operation).
  Using Dask may be a better solution with much more community support.
  As binning heavily relies on multi-threading for performance we would need to investigate whether that can leverage Dask.
