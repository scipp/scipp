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
Instead we publish new releases when major feature additions or breaking changes have gone into Scipp.

Upcoming milestones
-------------------

We are currently consolidating and simplifying Scipp's core data structures.
In particular:

- ``scipp.Dataset`` will be restricted to items of matching dimensionality (``scipp.DataGroup`` can be used for other applications).
- Support for attributes will be removed from ``scipp.DataArray`` due to conceptual consistency issues.

Future direction and plans
--------------------------

We have a number of improvements in mind which we would like to consider and potentially introduce to Scipp.
At this point we have not prioritized, scheduled, or funded any of these:

- `dask <https://dask.org/>`_ support.
- Support for `numpy's array-function API <https://numpy.org/doc/stable/reference/arrays.classes.html#numpy.class.__array_function__>`_, for better interoperability.
- Experiment with moving more high-level functionality from C++ to Python.
  Could we wrap NumPy (or similar) arrays, or is the performance penalty too high?
- Better support for structured dtypes and spatial dtypes.
