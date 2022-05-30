.. _roadmap:

Roadmap
=======

Overview
--------

The overall plan for the coming releases is to:

- Consolidate the existing generic functionality.
- Add missing generic functionality to provide a more complex toolbox.
- Weed out usability hurdles, to ensure the library's interface is easy to use.

At this point we do not have a fixed release cycle.
Instead we publish new releases when major feature additions or breaking changes have gone into scipp.

Upcoming milestones
-------------------

- We plan to provide a new and more powerful, but mainly more predictable plotting module.
  The current behavior, which automatically resamples data that is not 1-D, will then be deprecated.
  Later on, the new plotting module will become the default, and the current implementation will be removed.
- The current implementation of propagation of uncertainties has a fatal flaw when broadcasting is involved, since scipp cannot track correlations.
  It is therefore likely that we will disable support for automatic broadcasts of operands with uncertainties in a future release.

Future direction and plans
--------------------------

We have a number of improvements in mind which we would like to consider and potentially introduce to scipp.
At this point we have not prioritized, scheduled, or funded any of these:

- `dask <https://dask.org/>`_ support.
- Support for `numpy's array-function API <https://numpy.org/doc/stable/reference/arrays.classes.html#numpy.class.__array_function__>`_, for better interoperability.
