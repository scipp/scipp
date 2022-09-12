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
6. Support for ragged data (event data).

Aside from these concrete requirements, the central part that Jupyter would play in our software strategy was not anticipated in 2019.
At the time we mostly envisioned use through Python scripts or specialized GUIs.

## Scipp status

Scipp implements all the requirements listed above.
Essentially all operations are multi-threaded using TBB.
Some aspects have evolved significantly.
For example, the support for ragged data is simpler and more natural than the initial design in 2019.
The physical unit system is based on an external C++ library, `LLNL/units`.

A lot of additional functionality has been implemented as well.
It should be highlighted that all this is pure Python:

- Coordinate transformations (`sc.transform_coords`).
- Jupyter visualization features (such as `_repr_html_`, SVG, and table views).
- Plotting in any number of dimensions (currently being moved into `plopp`).
- Loading NeXus files (`scippnexus`)
- Basic HDF5 I/O.
- ESS-specific workflows (`ess`).


## Outside world

- Awkward array: first releases in 2018, 1.0 December 2020
- NEP18 `__array_function__` experimental in numpy-1.16 (Jan 2019), default in numpy-1.17 (July 2019)
- xarray proposal September 2021, https://zenodo.org/record/5484176
- xarray duck array wrapping (ongoing project): https://github.com/pydata/xarray/projects/2
- Awkward array backend discussion https://github.com/pydata/xarray/issues/4285
- Pandas `IntervalIndex` and `Interval`, first introduced in pandas-0.20 (May 2017) modified through 2019.
  Partially what we would want for bin-edges, but does not support math operations, i.e., not directly suitabel for coordinate transformations
- `xarray.DataTree` on the roadmap is relevant for us, e.g., for representing data from NeXus files

- dask

- DataArray/Dataset code code be written in Python... (almost) everything expressed via `transform` already

- wrapping numpy array to avoid copies (not serious issue NOW)... but may not work due to reference loops?

- uncertainty propagation less relevant... will disably typical costly cases anyway, and even if not, we can still keep `transform`?
- unit: ...
- masks: more difficult to handle in ops... but could use numpy.ma? ... actually scipp is quite slow here since it currently makes a copy to apply the mask in reduction ops, this faster than numpy.ma thanks to multi threading, but much less speedup than without mask
  could use dask.array?
- awkard array for binned data, overlay with ucstom API? what about threading and in-place ops?
- interfacing C++:
  - Mantid no longer relevant, or can use Python
  - special inelastic data structures... can't be more complicated than what Awkward does?
- https://github.com/ContinuumIO/dask-awkward is being worked on
- bin edges not in xarray... but do we want to use xarray, or just rewrite in Python?


- more code written in Python already now. Continue that (transform_coords, ...)

- https://github.com/xarray-contrib/pint-xarray is experimental

- keep getting asked: lazy loading

- Even if we switched to xarray, dask, awkward it is not implied that we have less work.
  Making an integration of these technologies is challenging and required continuous work.
  Would benefit from upstream improvements however.

- May be easier to hire Python devs?

- open issues related to attrs

- array API

## TODO

- understand if xarray coords with/without index can support our needs with coords/attrs
- think more about pandas

## Future?

These would potentially be of wider interest to the community, i.e., more likely to get support, provided that all is interoperable:

- implement dim-array with named dims (https://github.com/pydata/xarray/blob/main/xarray/core/variable.py and https://docs.xarray.dev/en/stable/roadmap.html#labeled-array-without-coordinates and https://github.com/pydata/xarray/issues/3981)
- implement multi-mask-array that works with xarray for mask support?
- implement array-like over awkward array that works with xarray (nest in multi-mask-array)
- implement array-like for bin edges

See also pydata/xarray#5648 Duck array compatibility meeting
