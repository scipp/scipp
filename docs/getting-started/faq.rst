.. _faq:

Frequently Asked Questions
==========================

Why is xarray not enough?
-------------------------

For our application (handling of neutron-scattering data, which is so far mostly done using `Mantid <https://mantidproject.org>`_), xarray is currently missing a number of features:

- Handling of physical units.
- Propagation of uncertainties.
- Support for histograms, i.e., bin-edge axes, which are by 1 longer than the data extent.
- Support for event data, a particular form of sparse data.
  More concretely, this is essentially a 1-D (or N-D) array of random-length lists, with very small list entries.
  This type of data arises in time-resolved detection of neutrons in pixelated detectors.
- Written in C++ for performance opportunities, in particular also when interfacing with our extensive existing C++ codebase.

Instead of using xarray we intend to provide converters and partial interoperability with :py:class:`xarray.Dataset`.
This lets us use, e.g., xarray's `powerful plotting functionality <https://xarray.pydata.org/en/stable/plotting.html>`_.

Why are you not just contributing to xarray?
--------------------------------------------

This is a valid criticism and at times we still feel uncertain about this choice.
The decision was made in the context of a larger project which needed to come to a conclusion within a couple of years, including all the items listed in the previous FAQ entry.
Essentially we felt that the list of additional requirements on top of what xarray provides was too long.
Effecting/contributing such fundamental changes to an existing framework is a long process and likely not obtained within the given time frame.
Furthermore, some of the requirements are unlikely to be obtainable within xarray.

We should note that at least some of our additional requirements, in particular physical units, are being pursued also by the xarray developers.

What about performance?
-----------------------

In its current state scipp does not support multi-threading and not all parts of the implementation are written to deliver optimal performance.
This was a deliberate choice for the early milestones.
We chose to focus first on delivering a decent API and tested implementation.
After these basics and the codebase have been consolidated, our roadmap contains more performance-related work and parallelization.
Rest assured that we have undertaken steps during the design and prototyping process to ensure that the architecture and data structures are compatible with higher performance needs.
