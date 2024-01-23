ADR 0002: Remove instrument view 2D projection options
======================================================

- Status: accepted
- Deciders: Neil, Simon
- Date: July 2020

Context
-------

The instrument view is a central tool for visualizing raw data collected by neutron detectors.
Traditionally, i.e., in Mantid, spherical and cylindrical projections have been used to map the detectors onto a plane.
In Scipp support for such 2D projections has also been added early on to ``scipp.neutron.instrument_view``.
However, this adds a fair bit of complexity to the code and does not even support volumetric detectors which are becoming more and more widespread at neutron scattering facilities.
At the same time, Scipp provides other functionality such as ``groupby`` or ``realign`` which can in many cases be used to create equivalent or better plots or raw data.

Decision
--------

Remove support for 2D projections from the instrument view.

Consequences
------------

Positive:
~~~~~~~~~

Instrument view code can be unified with other 3D plotting code:

- More time to focus on features that work with all kinds of detectors, including volumetric detectors with multiple layers.
- Improved maintainability.

Negative:
~~~~~~~~~

- Need to teach and document how other plotting tools can be used to achieve equivalent or alternative visualizations.
