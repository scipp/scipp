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

v0.5 (expected end 2020 / early 2021)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This release focusses on:

- Conceptual cleanup of ``Dataset`` and ``DataArray`` coordinates and masks, removing some ambiguities and inconsistencies.
- A complete replacement of the mechanism used for "event data" with a new, more generic and flexible binning mechanism.

v0.6
~~~~

Plans for this release include:

- More complete set of functionality, systematically eliminating missing operations, and handling of missing cases in existing operations.
- Performance improvements for binned data.
- Performance improvement for insertions of variables as dataset items or coords, by adopting a buffer-stealing approach by default.
