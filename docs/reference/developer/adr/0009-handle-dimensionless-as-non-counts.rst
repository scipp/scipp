ADR 0009: Handle dimensionless as non-counts
============================================

- Status: accepted
- Deciders: Jan-Lukas, Neil, Owen, Simon
- Date: 2021-09-01

Context
-------

Repeatedly there is doubt over how dimensionless quantities should be handled in operations that deal with counts.
This is primarily ``rebin`` and ``histogram``, but the effect is most visible in ``plot`` which internally rebins or resamples data.

There is no "counts" unit in the SI base unit system but we have included "counts" in the Scipp unit system from early on, since distinguishing dimensionless quantities from (neutron) counts is often essential.
For example, an explicit unit for "counts" prevents addition of "counts" with a ratio of counts.

Should "dimensionless" be handled similar to counts?
Let us consider which quantities may be dimensionless, or how they arise:

- `Wikipedia's list of dimensionless quantities <https://en.wikipedia.org/wiki/List_of_dimensionless_quantities>`_ is long and none of them appears to be "count-like".
- Dimensionless quantities naturally arise when we compute the ratio of two quantities with identical units.
  Treating ratios as counts in, e.g., ``rebin`` is obviously incorrect.

Decision
--------

Dimensionless should never by handled as if it was a counts-unit.

Consequences
------------

Positive:
~~~~~~~~~

- Fixes misleading and dangerous behavior.

Negative:
~~~~~~~~~

- User intervention is required when ingesting data into Scipp from a source that does not support a "counts" unit but uses dimensionless instead.
