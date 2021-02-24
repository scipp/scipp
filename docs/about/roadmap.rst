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

v0.6
~~~~

- More flexible support for physical units, instead of the current system which is limited to a hard-coded list of units.
  Along with such as change we also add support for unit scales, e.g., to support `mm` in addition to `m` for lengths.
- Better datetime support.

v0.7
~~~~

- ``scipp.neutron`` will be removed with this release, to be replaced by a standalone ``scippneutron`` package

v0.8
~~~~

- Reconsider scipp's approach to sharing data between objects.
  Our current approach is to always copy, which prevents unintentional modification of data of one object via another objects.
  However, this introduces a number of other ownership and lifetime issues, and is unlike the behavior of, e.g., Python's `dict` or xarray's data array objects.

Future direction and plans
--------------------------

We have a number of improvements in mind which we would like to consider and potentially introduce to scipp.
At this point we have not prioritized or scheduled any of these:

- More complete set of functionality, systematically eliminating missing operations, and handling of missing cases in existing operations.
- Native I/O for important file types.
  In our community this included first and foremost `NeXus <https://www.nexusformat.org/>`_ files for neutron event data.
- Support for `numpy's array-function API <https://numpy.org/doc/stable/reference/arrays.classes.html#numpy.class.__array_function__>`_, for better interoperability.
- `dask <https://dask.org/>`_ support.
