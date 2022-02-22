.. _release-notes:

Release Notes
=============

v0.13.0 (unreleased)
--------------------

Features
~~~~~~~~

Breaking changes
~~~~~~~~~~~~~~~~

* Changed format of scipp native HDF5 files to allow for UTF-8 characters in coord names; migration script: ``tools/migration/scipp-0.13-hdf5-files.py`` `#2463 <https://github.com/scipp/scipp/pull/2463>`_.

Bugfixes
~~~~~~~~

Deprecations
~~~~~~~~~~~~

Stability, Maintainability, and Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
and Jan-Lukas Wynen :sup:`a`

v0.12.0 (February 2022)
-----------------------

Features
~~~~~~~~

* Added support for :py:func:`abs` to :py:class:`scipp.DataArray`, :py:class:`scipp.Dataset`, :py:class:`scipp.Variable` `#2382 <https://github.com/scipp/scipp/pull/2382>`_.
* :py:func:`scipp.bin` will now also look at attributes to perform the binning, instead of only looking at the coordinates `#2388 <https://github.com/scipp/scipp/pull/2388>`_.
* Added :py:func:`scipp.squeeze` to remove length-1 dimensions `#2385 <https://github.com/scipp/scipp/pull/2385>`_.
* Switched to new system for runtime configuration, see the corresponding section in the reference documentation `#2389 <https://github.com/scipp/scipp/pull/2389>`_.
* Added :py:func:`scipp.midpoints` which can be used to compute bin-centers `#2404 <https://github.com/scipp/scipp/pull/2404>`_.
* :py:func:`scipp.fold` now supports ``-1`` for one of the dimension sizes, to indicate automatic shape `#2414 <https://github.com/scipp/scipp/pull/2414>`_.
* Added ``quiet`` argument to :py:func:`scipp.transform_coords` `#2420 <https://github.com/scipp/scipp/pull/2420>`_.
* Added support for strides for positional slicing. Negative strides are not supported for now `#2423 <https://github.com/scipp/scipp/pull/2423>`_.
* :py:func:`scipp.zeros_like`, :py:func:`scipp.ones_like`, :py:func:`scipp.empty_like`, and :py:func:`scipp.full_like` now accept and return data arrays `#2425 <https://github.com/scipp/scipp/pull/2425>`_.
* Added the ``.rename`` method to :py:class:`scipp.Variable`, :py:class:`scipp.DataArray`, and :py:class:`scipp.Dataset` that renames both dimensions and coords/attrs at the same time `#2428 <https://github.com/scipp/scipp/pull/2428>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* The configuration option ``config['plot']['padding']`` is now called ``config['plot']['bounding_box']``, no action is required if it was never modified `#2389 <https://github.com/scipp/scipp/pull/2389>`_.
* :py:func:`scipp.rebin` now applies masks that depend on the rebinning dimension to avoid "growing" effect on masks which previously resulted in masking more than intended `#2383 <https://github.com/scipp/scipp/pull/2383>`_.
* The ``unit`` attribute now distinguishes ``None`` from ``dimensionless`` `#2395 <https://github.com/scipp/scipp/pull/2395>`_ and `#2417 <https://github.com/scipp/scipp/pull/2417>`_.

  * For number-like dtypes the default is ``dimensionless`` (unchanged) whereas all other dtypes such as strings default to ``None`` (new).
  * ``bool`` (used for masks) is not considered number-like and the default unit is ``None``.
  * Comparison operator results now use ``None`` as unit of their output.
  * When integers without unit are used as indices, the new :py:func:`scipp.index` can be used to create a scalar variable without unit.

* :py:func:`scipp.spatial.rotations_from_rotvecs` has been changed to taking a variable input rather than separate values/dims/units.
* ``scipp.NotFoundError`` error has been replaced with ``KeyError`` in Python `#2416 <https://github.com/scipp/scipp/pull/2416>`_.
* Some functions now raise ``scipp.DimensionError`` instead of ``scipp.NotFoundError`` `#2416 <https://github.com/scipp/scipp/pull/2416>`_.

Bugfixes
~~~~~~~~

* Fix :py:func:`scipp.full`, which previously converted or attempted to convert all values to ``float64`` unless the ``dtype`` was specified explicitly `#2395 <https://github.com/scipp/scipp/pull/2395>`_.
* Fix :py:func:`scipp.transform_coords` for sliced binned data, used to raise an exception `#2406 <https://github.com/scipp/scipp/pull/2406>`_.
* Fix 1d plots y-axis limits when toggling log-scale with buttons and fix 2d plot colorbar when Inf values `#2426 <https://github.com/scipp/scipp/pull/2426>`_.
* Fix bug leading to datasets dims/shape modification after failed item insertion `#2437 <https://github.com/scipp/scipp/pull/2437>`_.
* Fix a plotting bug when 2d data has y coord associated with dimension x `#2446 <https://github.com/scipp/scipp/pull/2446>`_.
* Fix plotting with custom tick labels of ``dtype=datetime64`` which previously raised an exception `#2449 <https://github.com/scipp/scipp/pull/2449>`_.

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
Tom Willemsen :sup:`b, c`\ ,
and Jan-Lukas Wynen :sup:`a`

v0.11.0 (January 2022)
----------------------

Features
~~~~~~~~

* Added new datatypes for representing spatial transformations: ``affine_transform3``, ``rotation3``, and ``translation3``. Each of these can be combined and applied to the existing vector datatype.
* Added support for slicing without specifying a dimension (only for 1-D objects) `#2321 <https://github.com/scipp/scipp/pull/2321>`_.
* Added :py:func:`scipp.interpolate.interp1d` as convenience wrapper of the scipy functions of the same name `#2324 <https://github.com/scipp/scipp/pull/2324>`_.
* Added :py:func:`scipp.integrate.trapezoid` and :py:func:`scipp.integrate.simpson` as convenience wrappers of the scipy functions of the same name `#2324 <https://github.com/scipp/scipp/pull/2324>`_.
* Added ``unit`` property to ``obj.bins`` for getting and setting unit of bin elements `#2330 <https://github.com/scipp/scipp/pull/2330>`_.
* Added :py:func:`scipp.optimize.curve_fit` as convenience wrappers of the scipy function of the same name `#2350 <https://github.com/scipp/scipp/pull/2350>`_.
* Added :py:func:`scipp.signal.butter` and :py:func:`scipp.signal.sosfiltfilt` as wrappers of the scipy functions of the same name `#2356 <https://github.com/scipp/scipp/pull/2356>`_.
* Added :py:func:`scipp.Variable.to` and :py:func:`scipp.DataArray.to` as a convenience function for simultaneously converting dtype and units.
* Added :py:func:`scipp.datetime`, :py:func:`scipp.datetimes`, and :py:func:`scipp.epoch` to conveniently construct variables containing datetimes `#2360 <https://github.com/scipp/scipp/pull/2360>`_.
* Added ``camera`` option for 3-D scatter plots to control camera position and the point the camera is looking at `#2361 <https://github.com/scipp/scipp/pull/2361>`_.
* Allow more interoperability between :py:class:`scipp.DType` and other ways of encoding dtypes `#2373 <https://github.com/scipp/scipp/pull/2373>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* Minor change in behavior of dimension renaming in ``transform_coords``, new behavior documented in `Coordinate transformations <../user-guide/coordinate-transformations.rst>`_. Most use cases are unaffected. `#2319 <https://github.com/scipp/scipp/pull/2319>`_.
* ``sc.spatial.transform.as_rotvec`` has been moved to :py:func:`scipp.spatial.rotation_as_rotvec`
* ``sc.spatial.transform.from_rotvec`` has been moved to :py:func:`scipp.spatial.rotations_from_rotvecs`, and now returns a rotation data type rather than a matrix.
  For consistency with other transformation creation functions, ``rotations_from_rotvecs`` now takes ``values``, ``dims``, and ``unit`` separately.
* The matrix dtype ``matrix_3_float64`` has been renamed to ``linear_transform3``, and should now be constructed with :py:func:`scipp.spatial.linear_transform`.
* The vector dtype ``vector_3_float64`` has been renamed to ``vector3``.
* Scipp's logger is no longer preconfigured, this has to be done by the user `#2372 <https://github.com/scipp/scipp/pull/2372>`_.
* Data types are now attributes of the :py:class:`scipp.DType` class and the ``scipp.dtype`` module has been removed `#2373 <https://github.com/scipp/scipp/pull/2373>`_.

Bugfixes
~~~~~~~~

* Fix coordinate and attribute comparisons to treat NaN (not-a-number) values as equal, which previously prevented most operations with data arrays or datasets that contained NaN values in their coordinates or attributes `#2331 <https://github.com/scipp/scipp/pull/2331>`_.
* Fix incorrect result from ``sc.bin`` when rebinning an inner dimension to a multi-dimensional coordinate `#2355 <https://github.com/scipp/scipp/pull/2355>`_.

Deprecations
~~~~~~~~~~~~

* ``sc.matrix`` and ``sc.matrices`` have been deprecated in favour of :py:func:`scipp.spatial.linear_transform` and :py:func:`scipp.spatial.linear_transforms`, and will be removed in a future release of scipp.
* The previously deprecated ``concatenate`` and ``groupby(..).concatenate`` have been removed. Use :py:func:`scipp.concat` and :py:func:`scipp.GroupByDataArray.concat` instead.

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
Tom Willemsen :sup:`b, c`\ ,
and Jan-Lukas Wynen :sup:`a`

v0.10.0 (December 2021)
-----------------------

Features
~~~~~~~~

* Add support for auto-completions for slices and metadata access in Jupyter `#2304 <https://github.com/scipp/scipp/pull/2304>`_.
* Start publishing ``pip`` packages to `PyPI <https://pypi.org/>`_ in addition to ``conda`` packages.

Bugfixes
~~~~~~~~

* Fix kernel crash or poor performance when using ``bin`` or ``da.bins.concat`` along an inner dimension in the presence of a long outer dimension `#2278 <https://github.com/scipp/scipp/pull/2278>`_.
* Improved the rendering of cut slices in 3d plots where some strange artifacts could appear depending on the order in which the pixels were being drawn `#2287 <https://github.com/scipp/scipp/pull/2287>`_.

Deprecations
~~~~~~~~~~~~

* The deprecated ``events`` property has been removed.

Stability, Maintainability, and Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Improve a number of error messages `#2286 <https://github.com/scipp/scipp/pull/2286>`_ and `#2301 <https://github.com/scipp/scipp/pull/2301>`_, by Jan-Lukas Wynen.

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
Tom Willemsen :sup:`b, c`\ ,
and Jan-Lukas Wynen :sup:`a`

v0.9.0 (November 2021)
----------------------

Features
~~~~~~~~

* Added ``erf`` and ``erfc`` functions `#2195 <https://github.com/scipp/scipp/pull/2195>`_.
* Reduction operations such as ``sum``, ``nansum``, and ``cumsum`` of single precision (float32) data now use double precision (float64) internally to reduce rounding errors `#2218 <https://github.com/scipp/scipp/pull/2218>`_.
* Added ``bins_like`` for broadcasting dense variables to binned variables, e.g., for converting bin coordinates into event coordinates `#2225 <https://github.com/scipp/scipp/pull/2225>`_.
* ``groupby`` now also supports grouping by attributes instead of just by coordinates `#2227 <https://github.com/scipp/scipp/pull/2227>`_.
* Added ``concat`` to replace ``concatenate``. In contrast to the now deprecated ``concatenate``, ``concat`` supports concatenation of lists of objects instead of just two objects `#2232 <https://github.com/scipp/scipp/pull/2232>`_.
* Added ``dim`` property to ``Variable`` and ``DataArray`` `#2251 <https://github.com/scipp/scipp/pull/2251>`_.
* Added preconfigured logging support including a widget for output to notebooks `#2255 <https://github.com/scipp/scipp/pull/2255>`_, `#2267 <https://github.com/scipp/scipp/pull/2267>`_.
* Added ``reduce`` for setting up reductions across a list of inputs `#2267 <https://github.com/scipp/scipp/pull/2267>`_.
* Added toggle buttons for hiding/showing the 3d axes and outline box in 3d plots `#2265 <https://github.com/scipp/scipp/pull/2265>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* Name changes of arguments of ``transform_coords`` `#2267 <https://github.com/scipp/scipp/pull/2267>`_.
* Graphs in ``transform_coords`` must now name all outputs of functions or else the outputs are discarded `#2267 <https://github.com/scipp/scipp/pull/2267>`_.

Bugfixes
~~~~~~~~

* Fix bugs in ``rebin`` if data and/or edges had strides other than 1 along rebinned dimension, typically only occurring with multi-dimensional (ragged) coordinates `#2211 <https://github.com/scipp/scipp/pull/2211>`_.
* Fix exception that was thrown when importing empty datasets from HDF5 files using ``open_hdf5`` `#2216 <https://github.com/scipp/scipp/pull/2216>`_.
* Fix exception in ``astype`` when called with binned data that does not require conversion `#2222 <https://github.com/scipp/scipp/pull/2222>`_.
* Fix bug in ``concatenate`` that could lead to masks being shared with input rather than being copied `#2232 <https://github.com/scipp/scipp/pull/2232>`_.
* Fix exception in ``bin`` when binning in a new dimension but with an existing bin coord `#2237 <https://github.com/scipp/scipp/pull/2237>`_.
* Fix exception when plotting data with masks in presence of multi-dimensional coords `#2269 <https://github.com/scipp/scipp/pull/2269>`_.

Deprecations
~~~~~~~~~~~~

* ``concatenate`` and ``groupby(..).concatenate`` are deprecated. Use ``concat`` and ``groupby(..).concat`` instead.

Stability, Maintainability, and Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* More thorough unit testing for low-level code underlying most operations on dense as well as binned data `#1900 <https://github.com/scipp/scipp/pull/1900>`_, by Jan-Lukas Wynen.
* Switch to Jupyter Book theme for documentation, avoiding a number of issues with the RTD theme `#2233 <https://github.com/scipp/scipp/pull/2233>`_, by Neil Vaytet.
* Added `benchmark suite <https://scipp.github.io/scipp-benchmarks/>`_ to allow for identification of performance regressions, by Samuel Jones.
* Added `code coverage <https://scipp.github.io/scipp-coverage/>`_ tooling to allow for identification of dark spots in our test coverage, by Simon Heybrock.

Contributors
~~~~~~~~~~~~

Owen Arnold :sup:`b, c`\ ,
Simon Heybrock :sup:`a`\ ,
Samuel Jones :sup:`b`\ ,
Neil Vaytet :sup:`a`\ ,
Tom Willemsen :sup:`b, c`\ ,
and Jan-Lukas Wynen :sup:`a`\

v0.8.3 (September 2021)
-----------------------

Bugfixes
~~~~~~~~

* Fix serious correctness bug in ``groupby.sum`` and ``groupby.mean`` `#2200 <https://github.com/scipp/scipp/pull/2200>`_.

v0.8.0 (September 2021)
-----------------------

Features
~~~~~~~~

* Added ``sizes`` argument to ``zeros``, ``ones``, and ``empty`` variable creation functions `#1951 <https://github.com/scipp/scipp/pull/1951>`_.
* Slicing syntax now supports ellipsis, e.g., ``da.data[...] = var`` `#1961 <https://github.com/scipp/scipp/pull/1961>`_.
* Added bound method equivalents to many free functions which take a single Variable or DataArray `#1969 <https://github.com/scipp/scipp/pull/1969>`_.
* Variables can now be constructed directly from multi dimensional lists and tuples `#1977 <https://github.com/scipp/scipp/pull/1977>`_.
* Plotting 1-D event data is now supported `#2018 <https://github.com/scipp/scipp/pull/2018>`_.
* Add ``transform_coords`` for (multi-step) transformations based on existing coords, with support of event coords `#2058 <https://github.com/scipp/scipp/pull/2058>`_.
* Add ``from_pandas`` and ``from_xarray`` for conversion of pandas dataframes, xarray data arrays and dataset to scipp objects `#2054 <https://github.com/scipp/scipp/pull/2054>`_.
* Added ``full`` and ``full_like`` variable creation functions `#2069 <https://github.com/scipp/scipp/pull/2069>`_.
* ``islinspace`` can now take multi-dimensional variables as long as you pass the dimension to be checked `#2094 <https://github.com/scipp/scipp/pull/2094>`_.
* Added a power function and support for the ``**`` operator `#2083 <https://github.com/scipp/scipp/pull/2083>`_.
* Binned data now has a ``mean`` method as well as ``sum``, which returns the mean of each element within a bin.
* Add ``scipp.constants`` module for physical constants `#2101 <https://github.com/scipp/scipp/pull/2101>`_.
* Add ``scipp.spatial.transform`` module providing ``from_rotvec`` and ``as_rotvec`` `#2102 <https://github.com/scipp/scipp/pull/2102>`_.
* Add ``cross`` function to complementing existing ``dot`` function `#2109 <https://github.com/scipp/scipp/pull/2109>`_.
* The ``fields`` property of structured variables (vector and matrix dtypes) can now be iterated and provides dict-like access `#2116 <https://github.com/scipp/scipp/pull/2116>`_.
* Add ``where`` function.
* Unary operations such as ``sin`` are now available for datasets as well `#2112 <https://github.com/scipp/scipp/pull/2112>`_.
* Add rounding functions ``floor``, ``ceil``, and ``round`` that performs similarly to numpy equivelants `#2147 <https://github.com/scipp/scipp/pull/2147>`_.

Breaking changes
~~~~~~~~~~~~~~~~

.. include:: <isonum.txt>

* Changed names of arithmetic functions to match numpy's names: ``plus`` |rarr| :func:`scipp.add`, ``minus`` |rarr| :func:`scipp.subtract`, ``times`` |rarr| :func:`scipp.multiply` `#1999 <https://github.com/scipp/scipp/pull/1999>`_.
* Changed Variable init method, all arguments are keyword-only, special overloads which default-initialize data were removed `#1994 <https://github.com/scipp/scipp/pull/1994>`_.
* The ``axes`` keyword arg of ``plot`` has been removed.
  Use ``labels`` to define labels and ``transpose`` to transpose instead `#2018 <https://github.com/scipp/scipp/pull/2018>`_.
* The ``redraw`` method of plots does not support replacing data variables of data arrays any more, but only updates of data values `#2018 <https://github.com/scipp/scipp/pull/2018>`_.
* Made most arguments of ``DataArray`` and ``Dataset`` constructors keyword-only `#2008 <https://github.com/scipp/scipp/pull/2008>`_.
* Made arguments of many functions keyword-only, e.g. constructors of ``DataArray`` and ``Dataset``, ``bin``, and ``atan2`` `#1983 <https://github.com/scipp/scipp/pull/1983>`_, `#2008 <https://github.com/scipp/scipp/pull/2008>`_, `#2012 <https://github.com/scipp/scipp/pull/2012>`_.
* ``astype`` and ``to_unit`` now copy the input by default even when no transformation is required, use the new ``copy`` argument to avoid `#2016 <https://github.com/scipp/scipp/pull/2016>`_.
* ``rename_dims`` does not rename dimension-coords any more, only dims will be renamed `#2058 <https://github.com/scipp/scipp/pull/2058>`_.
* ``rename_dims`` does not modify the input anymore but returns a new object with renamed dims `#2058 <https://github.com/scipp/scipp/pull/2058>`_.
* ``issorted`` and ``islinspace`` return a variable of type boolean instead of a boolean `#2094 <https://github.com/scipp/scipp/pull/2094>`_.
* Multi-dimensional coordinates are no longer associated with their inner dimension, which affects the behavior when slicing:
  Such coords will no longer be turned into attributes during a slice operation on the inner dimension `#2098 <https://github.com/scipp/scipp/pull/2098>`_.
* ``buckets.map(histogram, da.data, 'time')`` has been replaced by ``lookup(histogram, 'time')[da.bins.coords['time']]`` `#2112 <https://github.com/scipp/scipp/pull/2112>`_.
* ``rebin`` and ``histogram`` now support any unit. ``plot()`` guesses which resampling mode to use and provides a button to toggle this `#2180 <https://github.com/scipp/scipp/pull/2180>`_.

Bugfixes
~~~~~~~~

* Various fixes in ``plot``, see  `#2018 <https://github.com/scipp/scipp/pull/2018>`_ for details.
* Operations with Python floats to long interpret the float as 32-bit float `#2101 <https://github.com/scipp/scipp/pull/2101>`_.
* Multi-dimensional bin-edge coordinates may now be edges for a non-inner dimension `#2098 <https://github.com/scipp/scipp/pull/2098>`_.

Deprecations
~~~~~~~~~~~~

* The ``events`` property is deprecated and is scheduled for removal with v0.9.

Contributors
~~~~~~~~~~~~

Owen Arnold :sup:`b, c`\ ,
Simon Heybrock :sup:`a`\ ,
Samuel Jones :sup:`b`\ ,
Greg Tucker :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
Tom Willemsen :sup:`b, c`\ ,
and Jan-Lukas Wynen :sup:`a`\

v0.7.2 (September 2021)
-----------------------

Bugfixes
~~~~~~~~

* Fix serious correctness bug in ``groupby.sum`` and ``groupby.mean`` `#2200 <https://github.com/scipp/scipp/pull/2200>`_.

v0.7.0 (June 2021)
------------------

Features
~~~~~~~~

* Licence changed from GPLv3 to BSD-3-Clause.
* Can now control the position and visibility of the legend in 1d plots with ``plot(da, legend={"show": True, "loc": 4})`` `#1790 <https://github.com/scipp/scipp/pull/1790>`_.
* Added ``zeros_like``, ``ones_like`` and ``empty_like`` functions `#1864 <https://github.com/scipp/scipp/pull/1864>`_.
* ``sort`` is now considerably faster for data with more rows `#1872 <https://github.com/scipp/scipp/pull/1872>`_.
* Added numpy-like ``linspace``, ``logspace``, ``geomspace``, and ``arange`` variable creation functions `#1871 <https://github.com/scipp/scipp/pull/1871>`_.
* ``to_unit`` now supports binned data `#1841 <https://github.com/scipp/scipp/pull/1841>`_.
* ``broadcast`` now returns a view instead of copying `#1861 <https://github.com/scipp/scipp/pull/1861>`_.
* ``fold`` now returns a view instead of copying `#1861 <https://github.com/scipp/scipp/pull/1861>`_.
* ``fold`` now works for binned data `#1861 <https://github.com/scipp/scipp/pull/1861>`_.
* ``flatten`` now returns a view of the input buffer if the input data is contiguous `#1861 <https://github.com/scipp/scipp/pull/1861>`_.
* Add ``redraw`` method to plot which enables an easy update on a figure when the underlying data has been modified `#1907 <https://github.com/scipp/scipp/pull/1907>`_.
* Several improvements for work with (3-D position) vectors and (3-D rotation) matrices `#1905 <https://github.com/scipp/scipp/pull/1905>`_:

  * Add creation functions ``vector``, ``vectors``, ``matrix``, ``matrices``.
  * Direct creation and initialization of 2-D (or higher) arrays of matrices and vectors is now possible from numpy arrays.
  * Fix extremely slow initialization of array of vectors or matrices from numpy arrays.
  * The ``values`` property now returns a numpy array with ``ndim+1`` (vectors) or ``ndim+2``` (matrices) axes, with the inner 1 (vectors) or 2 (matrices) axes corresponding o the vector or matrix axes.
  * Vector or matrix element can now be accessed and modified directly using the new ``fields`` property of ``Variable``.
    The ``fields`` property provides properties ``x``, ``y``, ``z`` (for variables containing vectors) or ``xx``, ``xy``, ..., ``zz`` (for matrices) that can be read as well as set.

* Reduction operations such as ``sum`` and ``mean`` are now also multi-threaded and thus considerably faster `#1923 <https://github.com/scipp/scipp/pull/1923>`_.

Bugfixes
~~~~~~~~

* Profile plots are no longer disabled for binned data with only a single bin in the 3rd dimension `#1936 <https://github.com/scipp/scipp/pull/1936>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* Scipp's data structures now behave mostly like normal nested Python objects, i.e., copies are shallow by default `#1823 <https://github.com/scipp/scipp/pull/1823>`_.
* ``filter`` and ``split`` removed. Identical functionality can be achieved using ``groupby`` and/or slicing.
* ``reshape`` has been removed. Use ``fold`` and ``flatten`` instead `#1861 <https://github.com/scipp/scipp/pull/1861>`_.
* ``geometry.x``, ``geometry.y``, and ``geometry.z`` have been removed. Use the ``Variable.fields`` property instead `#1925 <https://github.com/scipp/scipp/pull/1925>`_.

Contributors
~~~~~~~~~~~~

Owen Arnold :sup:`b, c`\, 
Simon Heybrock :sup:`a`\,
Matthew D. Jones :sup:`b, c`\,
Neil Vaytet :sup:`a`\,
and Jan-Lukas Wynen :sup:`a`\

v0.6.1 (April 2021)
-------------------

Bugfixes
~~~~~~~~

* ``map`` and ``scale`` operations as well as ``histogram`` for binned data now also work with ``datetime64`` `#1834 <https://github.com/scipp/scipp/pull/1834>`_.
* ``bin`` now works on previously binned data with 2-D edges, even if the outer dimensions(s) are not rebinned `#1836 <https://github.com/scipp/scipp/pull/1836>`_.
* ``bin`` and ``histogram`` now work with ``int32``-valued coordinates and support binning with ``int64``- or ``int32``-valued bin edges.

v0.6.0 (March 2021)
-------------------

Features
~~~~~~~~

* Add ``sizes`` properties, an ordered ``dict`` to lookup the size of a specific dimension `#1636 <https://github.com/scipp/scipp/pull/1636>`_.
* Add access to data, coords, attrs, and masks of bins via the ``bins`` property, e.g., ``obj.bins.coords['x']`` `#1752 <https://github.com/scipp/scipp/pull/1752>`_.
* Add more functions for data arrays that were previously available only for variables `#1638 <https://github.com/scipp/scipp/pull/1638>`_ `#1660 <https://github.com/scipp/scipp/pull/1660>`_.
* Add named versions of operators such as ``logical_and`` `#1660 <https://github.com/scipp/scipp/pull/1660>`_.
* Add ``to_unit`` to converter variables, e.g., between ``m`` and ``mm`` `#1756 <https://github.com/scipp/scipp/pull/1756>`_.
* Add modulo operations `#1660 <https://github.com/scipp/scipp/pull/1660>`_.
* Add scaling operations for Variables of type ``vector_3_float64``.
* ``sum`` and ``mean`` implemented for Variables of type ``vector_3_float64``.
* Add ``fold`` and ``flatten`` which allow to reshape dimensions of a Variable or DataArray `#1676 <https://github.com/scipp/scipp/pull/1676>`_.
* It is now possible to reshape a Variable also with ``sc.reshape(var, sizes={'x': 2, 'y': 3})``, in addition to ``sc.reshape(var, dims=['x', 'y'], shape=(2, 3))``.
* Add ``ones`` and ``empty`` creation functions, similar to what is known from numpy `#1732 <https://github.com/scipp/scipp/pull/1732>`_.
* ``scipp.neutron`` has been removed and is replaced by `scippneutron <https://scipp.github.io/scippneutron>`_
* ``scipp.neutron`` (now ``scippneutron``)

  * Support unit conversion to energy transfer, for inelastic TOF experiments `#1635 <https://github.com/scipp/scipp/pull/1635>`_.
  * Support loading/converting Mantid ``WorkspaceGroup``, this will produce a ``dict`` of data arrays `#1654 <https://github.com/scipp/scipp/pull/1654>`_.
  * Fixes to support loading/converting ``McStasNexus`` files `#1659 <https://github.com/scipp/scipp/pull/1659>`_.
* ``isclose`` added (``is_approx`` removed). Fuzzy data comparison ``isclose`` is an analogue to numpy's ``isclose``
* ``stddevs`` added `#1762 <https://github.com/scipp/scipp/pull/1762>`_.

* Support for datetime

  * ``sc.dtype.datetime64`` with copy-less casting between numpy and scipp where possible. `#1639 <https://github.com/scipp/scipp/pull/1639>`_
  * Binning with datetime edges `#1739 <https://github.com/scipp/scipp/pull/1739>`_

Breaking changes
~~~~~~~~~~~~~~~~

* The ``plot`` module has been renamed to ``plotting``, and plotting is now achieved via ``sc.plot(data)``. Wrapper functions ``superplot``, ``image``, and ``scatter3d`` have been dropped `#1696 <https://github.com/scipp/scipp/pull/1696>`_.
* Properties ``dim``, ``begin``, ``end``, and ``data`` of the ``bins`` property of and object are now accessible as a dict via ``obj.bins.constituents``.
* ``scipp.neutron`` has been removed and is replaced by `scippneutron <https://scipp.github.io/scippneutron>`_
* ``scipp.neutron`` (now ``scippneutron``)
* ``is_equal`` renamed ``identical``
* ``is_linspace`` renamed ``islinspace``
* ``is_sorted`` renamed ``issorted``
* ``is_approx`` superseded by ``isclose``. ``is_approx`` removed.
* Removed support for facility-specific ``convert_with_calibration`` and ``load_calibration``

Contributors
~~~~~~~~~~~~

Matthew Andrew,
Owen Arnold,
Simon Heybrock,
Matthew D. Jones,
Andrew McCluskey,
Neil Vaytet,
and Jan-Lukas Wynen

v0.5.0 (January 2021)
---------------------

Features
~~~~~~~~

* New ``profile`` plotting functionality where one of the slider dimensions can be displayed as a profile in a subplot (1-D and 2-D projections only).
* Sliders have a thickness slider associated with them and can be used to show slices of arbitrary thickness.
* Can hide/show individual masks on plots.
* Can toggle log scale of axes and colorbar with buttons in figure toolbar.
* Add binned data support, replacing "event list" dtypes as well as "realign" support.
* Plotting of event data (binned data) with dynamic resampling with "infinite zoom" functionality.
* Value-based slicing support.
* Possibility to plot Scipp objects using ``my_data_array.plot()`` in addition to the classical ``plot()`` free function.
* Support for saving and loading scipp data structures to HDF5.
* More functions such as ``nanmean`` for better handling of special values such as ``INF`` and ``NaN``.
* TBB (multi-threading) support for MacOS.
* ``scipp.neutron``

  * Improved instrument view, e.g., with buttons to align camera with an axis.
  * Experiment logs (previously using Mantid's ``Run``) are now represented as native scipp objects, e.g., as scalar attributes holding a data array representing a time-series such as a temperature log.
  * Support conversion of ``mantid.MaskWorkspace``.

Breaking changes
~~~~~~~~~~~~~~~~

* ``Dataset`` does not have a ``masks`` property any more.
  Use ``ds['item'].masks`` instead.
* ``Dataset`` does not support attributes any more.
* ``DataArray`` and dataset item attributes are now are now handled as "unaligned" coords.
  Use ``ds['item'].coords`` or ``array.attrs`` to access these.
* API for log scale on axes and colors has changed.
  Use ``plot(da, scale={'tof': 'log'})`` to set a log scale on a coordinate axis, and use ``plot(da, norm='log')`` to have a log image colorscale or a log y axis on a 1d plot.
* ``vmin`` and ``vmax`` now represent absolute values instead of exponents when ``norm='log'``.
* The ``ipympl`` matplotlib backend is now required for using inside Jupyter notebooks.
  This has been added as a dependency.
  It is also the only interactive backend that works in JupyterLab.
* Removed support for ``event_list`` ``dtype``, use binned data instead.
* Removed support for "realigned" data. This is replaced by the more flexible and generic support for "binned" data.

Contributors
~~~~~~~~~~~~

Matthew Andrew,
Owen Arnold,
Thibault Chatel,
Simon Heybrock,
Matthew D. Jones,
Daniel Nixon,
Piotr Rozyczko,
Neil Vaytet,
and Jan-Lukas Wynen

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
* Plot with keyword argument ``collapse`` has been removed in favor of two more generic free functions that return a ``dict`` of data arrays that can then directly be passed to the ``plot`` function:

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

Contributing Organizations
--------------------------
* :sup:`a`\  `European Spallation Source ERIC <https://europeanspallationsource.se/>`_, Sweden
* :sup:`b`\  `Science and Technology Facilities Council <https://www.ukri.org/councils/stfc/>`_, UK
* :sup:`c`\  `Tessella <https://www.tessella.com/>`_, UK
