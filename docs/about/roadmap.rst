.. _roadmap:

Roadmap
=======

Overview
--------

As of autumn 2020 scipp is still in an "early preview" state.
The overall plan for the coming releases is to:

- Consolidate the existing generic functionality.
- Add missing generic functionality to provide a more complex toolbox.
- Weed out usability hurdles, to ensure the library's interface is easy to use.

At this point we do not have a fixed release cycle.
Instead we publish new releases when major feature additions or breaking changes have gone into scipp.

Upcoming milestones
-------------------

v0.5 (expected February 2021)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This release focusses on:

- Conceptual cleanup of ``Dataset`` and ``DataArray`` coordinates and masks, removing some ambiguities and inconsistencies.
- A complete replacement of the mechanism used for "event data" with a new, more generic and flexible binning mechanism.

v0.6
~~~~

Plans for this release include:

- More complete set of functionality, systematically eliminating missing operations, and handling of missing cases in existing operations.
- Performance improvements for binned data.
- Performance improvement for insertions of variables as dataset items or coords, by adopting a buffer-stealing approach by default.

Future direction and plans
--------------------------

We have a number of improvements in mind which we would like to consider and potentially introduce to scipp.
At this point we have not prioritized or scheduled any of these:

- More flexible support for physical units, instead of the current system which is limited to a hard-coded list of units.
  Along with such as change we would also like to add support for unit scales, e.g., to support `mm` in addition to `m` for lengths.
- Reconsider scipp's approach to sharing data between objects.
  Our current approach is to always copy, which prevents unintentional modification of data of one object via another objects.
  However, this introduces a number of other ownership and lifetime issues, and is unlike the behavior of, e.g., Python's `dict` or xarray's data array objects.
- Native I/O for important file types.
  In our community this included first and foremost `NeXus <https://www.nexusformat.org/>`_ files for neutron event data.
- Support for `numpy's array-function API <https://numpy.org/doc/stable/reference/arrays.classes.html#numpy.class.__array_function__>`_, for better interoperability.
- `dask <https://dask.org/>`_ support.
