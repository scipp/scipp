# Scipp as a C++ library and the scientific Python software ecosystem

## Purpose

The purpose of this document is an updated look at the decision to implement Scipp in C++, the developments in the Python ecosystem in the past couple of years, and how they may relate to the future of Scipp.

## Background

### Origins

Scipp was born out of a design proposal and prototype for a possible replacement of Mantid's workspaces, a hierarchy a classes for storing all kinds of data.
The design started in January 2018, resulting in the "Workspace 2.0" proposal towards the end of 2018.
With hesitant feedback from neutron-scattering facilities other than ESS, 2019 marked a shift in focus to develop the prototype into the stand-alone Scipp library.
At the time, the goal was to use this in combination *with* Mantid but not *in* Mantid.
This was in the context of a projected ESS hot-commissioning date some time in 2022.

### Initial requirements and boundary conditions

The high-level requirements in 2019 could be summarized as follows:

1. C++ core with Python bindings.
  This originates from a combination of aspects:
  - We considered it likely that we would eventually interface with Mantid on the C++ side to ensure maximum performance.
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
For example, the support for ragged data is simpler and more natural than the initial design in 2019.
The physical unit system is based on an external C++ library, `LLNL/units`.

Interoperability with the Python world is ensured by exposing arrays using the buffer protocol.
That is, Python libraries (including NumPy) that expect NumPy arrays can operate on these buffers.

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

In summary, Scipp may turn out to become a permanent in-house project (with hypothetical but possible adoption in a couple of neutron-scattering facilities in the future).
As such, while an improvement over many aspects of Mantid, at a long time scale it might suffer a similar fate:

- Tightly scoped support and adoption (both in-house) while the outside world moves on.
- Limited benefit derived from outside progress.
- Users and developers require specific training (albeit less than for Mantid).
- Risk of reaching a tipping point, at which a "big rewrite" is undertaken.

## Outside world

A brief review of the "outside world" reveals that the high-level Scipp requirements or being pursued by other projects as well.
A lot of this has been (and still is) happening since 2018/2019, i.e., some of this is parallel development and was not available as such during the infancy of the Scipp project.

- The [Python array API standard](https://data-apis.org/array-api/latest/) saw its first (non-draft) release in December 2021.
  This *standardize[s] functionality that exists in most/all array libraries and either is commonly used or is needed for consistency/completeness*.
  The goal is to address the fragmentation between libraries such as NumPy, Tensorflow, PyTorch, Dask, JAX, CuPy, MXNet, Xarray, and others.
- NEP18 added `__array_function__` (in addition to the previous `__array_ufunc__`), experimental in numpy-1.16 (Jan 2019), default in numpy-1.17 (July 2019).
- Based on `__array_function__`, Xarray could work on better [Pint](https://github.com/hgrecco/pint) support, resulting also in the experimental [pint-xarray](https://github.com/xarray-contrib/pint-xarray is experimental).
- The above is part of the wider "duck array wrapping" effort, see the [ongoing Xarray project](https://github.com/pydata/xarray/projects/2) and the [Duck array compatibility meeintg](https://github.com/pydata/xarray#5648).
- The [Xarray roadmap](https://docs.xarray.dev/en/stable/roadmap.html):
  - [Extracting xarray.Variable into a standalone package](https://docs.xarray.dev/en/stable/roadmap.html#labeled-array-without-coordinates) is a relevant development.
    It would allow for use of this component without the much larger Xarray (and Pandas) dependency.
    More details can be found in the related [proposal from September 2021](https://zenodo.org/record/5484176).
  - `xarray.DataTree` on the roadmap is relevant for us, e.g., for representing data from NeXus files.
    This goes beyond anything we have planned for Scipp, but tree like structures are encountered repeatedly, i.e., in neutron data reduction workflows, but have so far always been address by crude flattening of the structures, or be used of nested Python dicts.
- [Awkward array](https://github.com/scikit-hep/awkward) is a library for nested, variable-sized data.
  It was first released in 2018, reaching version 1.0 in December 2020.
  It is currently undergoing a rewrite, moving a lot of code to Python.
  It supports data structures similar to Scipp's *binned data* (possible limitations: not in-place, not multi-threaded except for the experimental [dask-awkward](https://github.com/ContinuumIO/dask-awkward)).
  The is an ongoing discussion for an [Awkward array backend](https://github.com/pydata/xarray/issues/4285) in Xarray, again based on what is possible thanks to `__array_function__` and the work going into duck array wrapping.
- TODO Xarray flexible indices development

## Hypothetical future of Scipp

Given the developments outlines above, we may consider if and how it might be possible to replace Scipp or parts of it using other Python libraries, and determine which piece, if any, are still missing or lacking.

The central feature we would need to rely on NumPy's duck array and `__array_ufunc__` and `__array_function__` protocols.
In the following we will refer to this simply as *duck array*.
We would need the following:

- Duck array with labeled dimensions.
  This is on the Xarray roadmap, moving `xarray.Variable` into a standalone package.
- Duck array with physical units.
  `Pint` appears to provide this by now.
- Duck array with uncertainties.
  The Python `uncertainties` packages provides this, but it is likely not useable since it tracks correlations and is not computationally feasible for our purposed.
  We would thus need to provide a simpler duck-array package.
  There are two relevant notes here:
  - Early on in the Scipp development we argued that uncertainty propagation based on NumPy array math may be 10x slower (for inplace `multiply` or `divide` operations) and we therefore need to implement this in C++.
    This may still be partially true, but this is a statement about the implementation of the *operation*, not the *data structure*.
    That is, we could still provide a Python-only duck array for this.
  - Multiplication and division operations with uncertainties and broadcast that commonly arose in neutron-scattering data reduction will be remove due to the unhandled introduction of correlations.
    See upcoming publication.
- Bin-edge coordinates are not supported in Xarray.
  However, Pandas' `IntervalIndex` and `Interval`, first introduced in pandas-0.20 (May 2017) and modified through 2019 partially cover our needs for bin-edges.
  It does not support math operations, i.e., is not directly suitable for coordinate transformations.
  This requires more investigation.
  One possibility may be a bin-edge duck array, optionally wrapping `IndervalIndex`.
- Scipp's masking feature supports a `dict` of masks, in contrast to `numpy.ma` which provides a single mask.
  Scipp applies masks selectively in reduction operations.
  This seems to be a simple case where we could provide a "multi mask" duck array handling the mask dict, forwarding, e.g., to `numpy.ma` for operations.
- Scipp's binned data could be represented as a duck array wrapping `awkward.array`.
  As more parties are interested in Xarray support there may be common interest in a joint development (inside or on top of `awkward`).

All of the above are essentially independent of each other.
That is, each of the items above may be supported as a standalone Python package.
Most of these may be carried by a large community, i.e., this part of Scipp's scope is no longer tied to Scipp itself.
The duck arrays could internally use `numpy`, but would equally well work with `dask.array`.

Aside from this "stack" of duck arrays, the remaining Scipp scope comprises:

- Binning, grouping, histogramming, and resampling operations.
  These would need to be provided as a library that can operate on "arbitrary" duck arrays.
- `scipp.DataArray` and `scipp.Dataset`.
  One option would be to simple use Xarray.
  API differences could either be rolled out to our codebase, or wrappers would be provided.
  A second option would be a Python-only implementation, built on the stack of duck arrays outlined above.
- Multi-threading is crucial for the single-node performance in neutron scattering data reduction workflows.
  NumPy does not multi-thread BLAS vector-vector operations so other solutions would need to be investigated.
  One option could be the existing Scipp framework for operations on `scipp.Variable`, adopted to work on any NumPy-like buffer.
  This could be another duck-array, wrapping an underlying array implementation but providing multi-threaded operations (if implemented, otherwise fallback to calling underlying array operation).
  Using Dask may be a better solution with much more community support.
  As binning heavily relies on multi-threading for performance we would need to investigate whether that can leverage dask.




# Unsorted notes (ignore)


- DataArray/Dataset code code be written in Python... (almost) everything expressed via `transform` already

- wrapping numpy array to avoid copies (not serious issue NOW)... but may not work due to reference loops?

- uncertainty propagation less relevant... will disably typical costly cases anyway, and even if not, we can still keep `transform`?
- masks: more difficult to handle in ops... but could use numpy.ma? ... actually scipp is quite slow here since it currently makes a copy to apply the mask in reduction ops, this faster than numpy.ma thanks to multi threading, but much less speedup than without mask
  could use dask.array?
- awkard array for binned data, overlay with ucstom API? what about threading and in-place ops?
- interfacing C++:
  - Mantid no longer relevant, or can use Python
  - special inelastic data structures... can't be more complicated than what Awkward does?

- more code written in Python already now. Continue that (transform_coords, ...)

- Even if we switched to xarray, dask, awkward it is not implied that we have less work.
  Making an integration of these technologies is challenging and required continuous work.
  Would benefit from upstream improvements however.

- May be easier to hire Python devs?

- open issues related to attrs

- understand if xarray coords with/without index can support our needs with coords/attrs
- think more about pandas
