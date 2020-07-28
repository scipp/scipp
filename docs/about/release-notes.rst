.. _release-notes:

Release Notes
=============

v0.4 (July 2020)
----------------

Features
~~~~~~~~

* New realign functionality.
* Support for event-filtering.
* Support for subtraction and addition for (realigned) event data.
* Non-range slicing changed to preserve coords as attrs rather than dropping
* ``scipp.neutron``: Instrument view with advanced geometry support, showing correct pixel shapes.
* Instrument view working on doc pages.
* Made it simpler to add new ``dtype`` and support ``transform`` for all types.
* Comparison functions such as ``less``, ``greater_equal``, ...
* ``all`` and ``any`` can work over all dimensions as well as explicitly provided dimension argument
* It is now possible to convert between Scipp objects and Python dictionaries using ``to_dict`` and ``from_dict``.
* New functions ``collapse`` and ``slices`` can be use to split one or more dimensions of a DataArray to a dict of DataArrays.
* You can now inspect the global object list of via the ``repr`` for scipp showing Datasets, DataArrays and Variables
* Internal cleanup and documentation additions.

Noteable bug fixes
~~~~~~~~~~~~~~~~~~

* Several fixes in the plotting (non-regular bins, colorbar limits, axes tick labels from unaligned coordinates, etc...)

Breaking changes
~~~~~~~~~~~~~~~~

* Coord and attributes names for neutron data have been standardized, now using hyphens instead of underscore, except for subscripts. Affected examples: ``pulse-time`` (previously ``pulse_times``), ``source-position`` (previously ``source_position``), ``sample-position`` (previously ``sample_position``), ``detector-info`` (previously ``detector_info``).
* ``scipp.neutron.load`` must use ``advanced_geometry=True`` option for loading ``detector-info`` and pixel shapes.
* Normalization of event data cannot be done directly any more, must use ``realign``.
* Plotting variances in 2D has been removed, and the API for using ``matplotlib`` axes has been simplified slightly, since we no longer have axes for variances:
  * Before: ``plot(..., mpl_axes={"ax": myax0, "cax": myax1})``
  * After: ``plot(..., ax=myax0, cax=myax1)``
* Plot with keyword argument ``collapse`` has been removed in favour of two more generic free functions that return a ``dict`` of data arrays that can then directly be passed to the ``plot`` function:
  * ``collapse(d, keep='x')`` slices all dimensions away to keep only ``'x'``, thus always returning 1D slices.
  * ``slices(d, dim='x')`` slices along dimension ``'x'``, returning slices with ``ndim-1`` dimensions contaiing all dimensions other than ``'x'``.

Contributors
~~~~~~~~~~~~

Owen Arnold,
David Fairbrother,
Simon Heybrock,
Daniel Nixon,
Pawel Ptasznik,
Piotr Rozyczko,
and Neil Vaytet


v0.3 (March 2020)
-----------------

* Many bug fixes and small additions
* Multi-threading with TBB for many operations.
* Performance improvements in hotspots
* Remove ``Dim`` labels in favor of plain strings. Connected to this, the ``labels`` property for data arrays and datasets has been removed. Use ``coords`` instead.
* Start to support ``out`` arguments (not everywhere yet)
* ``scipp.neutron``: Instrument view added

Contributors in this release:
Owen Arnold,
Simon Heybrock,
Daniel Nixon,
Dimitar Tasev,
and Neil Vaytet


v0.2 (December 2019)
--------------------

* Support for masks stored in ``DataArray`` and ``Dataset``.

* Support for ``groupby``, implementing a split-apply-combine approach as known from pandas.

* Enhanced support for event data:

  * Histogramming with "weighted" data.
  * Multiplication/division operators between event data and histogram.

* Enhanced plotting support:

  * Now focussing on ``matplotlib``.
  * Multi-dimensional plots with interactive sliders, and much more.

* Significant performance improvements for majority of operations. Typically performance is now in the same ballpark as what the memory bandwidth on a single CPU core can support.

* Fancy ``_repr_html_`` for quick views of datasets in Jupyter notebooks.

* Conda packages now also available for Windows.

* ``scipp.neutron`` gets improved converters from Mantid, supporting neutron monitors, sample information, and run information stored as attributes.

Contributors in this release:
Owen Arnold,
Igor Gudich,
Simon Heybrock,
Daniel Nixon,
Dimitar Tasev,
and Neil Vaytet


v0.1 (September 2019)
---------------------

This is the first official release of ``scipp``.
It is not yet meant for production-use, but marks a big step for us in terms of usability and features.
The API may change without notice in future releases.

Features:

* All key data structures (``Variable``, ``DataArray``, and ``Dataset``).
* Slicing.
* Basic arithmetic operations.
* Physical units.
* Propagation of uncertainties.
* Event data.

Limitations:

* Limited performance and no parallelization.
* Numerous "edge cases" not supported yet.
* While tested, probably far from bug-free.
