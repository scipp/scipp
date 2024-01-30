.. _release-notes:

Release Notes
=============


.. Template, copy this to create a new section after a release:

   vrelease
   --------

   Features
   ~~~~~~~~

   Breaking changes
   ~~~~~~~~~~~~~~~~

   Bugfixes
   ~~~~~~~~

   Documentation
   ~~~~~~~~~~~~~

   Deprecations
   ~~~~~~~~~~~~

   Stability, Maintainability, and Testing
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

vrelease
--------

Features
~~~~~~~~

* Added :func:`scipp.median`, :func:`scipp.nanmedian`, :func:`scipp.var`, :func:`scipp.nanvar`, :func:`scipp.std`, :func:`scipp.nanstd` `#3378 <https://github.com/scipp/scipp/pull/3378>`_.

Breaking changes
~~~~~~~~~~~~~~~~

Bugfixes
~~~~~~~~

Documentation
~~~~~~~~~~~~~

Deprecations
~~~~~~~~~~~~

Stability, Maintainability, and Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


v23.12.0
--------

Features
~~~~~~~~

* Add :func:`scipp.curve_fit` `#3303 <https://github.com/scipp/scipp/pull/3303>`_.
* Add hyperbolic functions such as :func:`scipp.sinh` `#3348 <https://github.com/scipp/scipp/pull/3348>`_.

Bugfixes
~~~~~~~~

* :func:`scipp.spatial.as_vectors` now works with input dtypes other than float64 `#3346 <https://github.com/scipp/scipp/pull/3346>`_.

v23.11.1
--------

Bugfixes
~~~~~~~~

* Fix a memory leak in functions such as :func:`scipp.bin` and :func:`scipp.group` that affects Scipp versions between 23.08.0 and 23.11.0 `#3342 <https://github.com/scipp/scipp/pull/3342>`_.

v23.11.0
--------

Features
~~~~~~~~

* Operations on binned variables with a data array content buffer now also work with slices of binned variables `#3282 <https://github.com/scipp/scipp/pull/3282>`_.
* Reduced per-bin overhead in many operations involving binned variables by about 20 milliseconds per 1 million bins `#3282 <https://github.com/scipp/scipp/pull/3282>`_.
* It is now possible to construct Scipp variables from Numpy arrays with up to 6 dimensions for arbitrary memory layouts and any number of dimensions for c-contiguous memory layouts.
  The limit used to be ``ndim <= 4``.
  This also improves performance in many cases `#3284 <https://github.com/scipp/scipp/pull/3284>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* All items of datasets must have the same dimensions now `#3199 <https://github.com/scipp/scipp/pull/3199>`_.
* The runtime configuration has been removed `#3254 <https://github.com/scipp/scipp/pull/3254>`_.
* The deprecated function ``open_hdf5`` and ``to_hdf5`` have been removed in favor of ``load_hdf5`` and ``save_hdf5`` `#3300 <https://github.com/scipp/scipp/pull/3300>`_.

Bugfixes
~~~~~~~~

* :func:`scipp.testing.assert_identical` now compares the alignment of coordinates `#3242 <https://github.com/scipp/scipp/pull/3242>`_.
* Fixed overflow in variances with large integers `#3262 <https://github.com/scipp/scipp/pull/3262>`_.
* Fixed reduction operations for data arrays containing vectors with masks `#3276 <https://github.com/scipp/scipp/pull/3276>`_.
* Fixed :func:`scipp.where` to work with most dtypes `#3276 <https://github.com/scipp/scipp/pull/3276>`_.
* Event-centric arithmetic now uses the correct dtype in the result `#3278 <https://github.com/scipp/scipp/pull/3278>`_.
* Fixed a serious bug where operations on, e.g., transposed binned data resulted in corrupt coord, mask, and attr values of the bin contents `#3282 <https://github.com/scipp/scipp/pull/3282>`_.
* ``min`` and ``max`` now return not-a-number if any input element is not-a-number instead of ignoring that element `#3299 <https://github.com/scipp/scipp/pull/3299>`_.

Documentation
~~~~~~~~~~~~~

* Removed the "What's new" page because it had not been updated in a while and Scipp changes less drastically now `#3296 <https://github.com/scipp/scipp/pull/3296>`_.

Deprecations
~~~~~~~~~~~~

* ``scipp.DataArray.attrs`` and all related methods and properties have been deprecated `#3227 <https://github.com/scipp/scipp/pull/3227>`_.

v23.08.0
--------

Features
~~~~~~~~

* Improve performance when binning or grouping into many bins `#3215 <https://github.com/scipp/scipp/pull/3215>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* Built-in legacy plotting support has been removed, as per prior deprecation, in favor of Plopp `#3200 <https://github.com/scipp/scipp/pull/3200>`_.

Bugfixes
~~~~~~~~

* Update units library to fix various small issues `#3203 <https://github.com/scipp/scipp/pull/3203>`_.

v23.07.0
--------

Features
~~~~~~~~

* Added alignment flag to ``Variable`` `#3144 <https://github.com/scipp/scipp/pull/3144>`_.
* Handle alignment of coordinates in operations `#3153 <https://github.com/scipp/scipp/pull/3153>`_.
* ``sc.compat.from_pandas`` now has additional arguments `#3169 <https://github.com/scipp/scipp/pull/3169>`_.
* Added ``sc.io.load_csv`` `#3169 <https://github.com/scipp/scipp/pull/3169>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* Coordinates are no longer converted to attributes when slicing or by ``transform_coords``.
  Instead, they become unaligned `#3153 <https://github.com/scipp/scipp/pull/3153>`_.

Bugfixes
~~~~~~~~

* Fixed label based indexing for arrays with a single element in the sliced dimension `#3171 <https://github.com/scipp/scipp/pull/3171>`_.
* Fixed ``scipp.show`` in the presence of bin-edge coords that have been sliced out `#3178 <https://github.com/scipp/scipp/pull/3178>`_.
* Fixed in-place mod function for float `#3179 <https://github.com/scipp/scipp/pull/3179>`_.

v23.05.0
--------

Features
~~~~~~~~

* :class:`scipp.Dataset` now supports ``assign_coords``, which updates or inserts the given coordinates to the :class:`scipp.Dataset` `#3110 <https://github.com/scipp/scipp/pull/3110/>`_.
* :class:`scipp.DataArray` now supports ``assign_coords``/ ``assign_masks``/ ``assign_attrs``, which updates or inserts the given coordinates/masks/attributes to the :class:`scipp.DataArray` `#3110 <https://github.com/scipp/scipp/pull/3110/>`_.
* Fixed performance issues for ``datetime64`` operations `#3123 <https://github.com/scipp/scipp/pull/3123/>`_.
* The HDF5 writer and reader now supports builtin types (e.g. ``int``, ``str``) and numpy types (e.g. ``np.ndarray``, ``np.int64``) as data group items `#3124 <https://github.com/scipp/scipp/pull/3124/>`_.
* :class:`scipp.DataGroup` reduction- and shape operations now handle items of lower dimensionality more consistently.
  Operations will thus work in more situations, rather than raise exceptions `#3136 <https://github.com/scipp/scipp/pull/3136/>`_.

Bugfixes
~~~~~~~~

* Fix bug in :func:`scipp.lookup` raising an exception in arithmetic operations with unrelated coords `#3112 <https://github.com/scipp/scipp/pull/3112>`_.

v23.03.2
--------

Bugfixes
~~~~~~~~

* Fix :func:`scipp.bins` to work with ``int32`` begin/end indices instead of just ``int64`` `#3116 <https://github.com/scipp/scipp/pull/3116>`_.

v23.03.1
--------

Features
~~~~~~~~

* Added ``__ifloordiv__`` to ``Variable`` and ``DataArray`` `#3101 <https://github.com/scipp/scipp/pull/3101>`_.

Bugfixes
~~~~~~~~

* Fix bug that caused ``scipp.testing.strategies.variables`` to produce negative variances `#3100 <https://github.com/scipp/scipp/pull/3100>`_.
* Fix the source of segmentation faults coming from caching the NumPy module in ``Variable.value`` `#3105 <https://github.com/scipp/scipp/pull/3105>`_.

Deprecations
~~~~~~~~~~~~

* ``scipp.geometry.position`` has been moved to ``scipp.spatial.as_vectors``, the original function is deprecated `#3094 <https://github.com/scipp/scipp/pull/3094>`_.
* ``scipp.geometry.rotation_matrix_from_quaternion_coeffs`` has been deprecated without replacement `#3094 <https://github.com/scipp/scipp/pull/3094>`_.

v23.03.0
--------

Features
~~~~~~~~

* Added new string-formatting options `#3017 <https://github.com/scipp/scipp/pull/3017>`_, `#3028 <https://github.com/scipp/scipp/pull/3028>`_.
* Added ``size`` property to :class:`scipp.Variable` and :class:`scipp.DataArray`.
* Added :func:`scipp.testing.assert_identical` `#3066 <https://github.com/scipp/scipp/pull/3066>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* `Plopp <https://scipp.github.io/plopp/>`_ is now the default plotting backend `#3076 <https://github.com/scipp/scipp/pull/3076>`_.
* ``open_hdf5`` and ``to_hdf5`` have been renamed to ``load_hdf5`` and ``save_hdf5``, respectively, `#3081 <https://github.com/scipp/scipp/pull/3081>`_.
* Removed ``scipp.matrix`` and ``scipp.matrices``. Use functions in ``scipp.spatial`` to construct more specialized variables, `#3098 <https://github.com/scipp/scipp/pull/3098>`_.

Bugfixes
~~~~~~~~

* Fix access to value of 0-D variable with "spatial transform" dtypes using the ``value`` property.
  Previously this raised an exception if the dtype was ``affine_transform3``, ``translation3``, or ``rotation3`` `#3033 <https://github.com/scipp/scipp/pull/3033>`_.
* Fix ``coords.keys()`` and ``coords.items()`` to compare underlying dicts (same for ``masks`` and ``attrs``) `#3064 <https://github.com/scipp/scipp/pull/3064>`_.

Documentation
~~~~~~~~~~~~~

Deprecations
~~~~~~~~~~~~

Stability, Maintainability, and Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Switched to black for Python code formatting `#3070 <https://github.com/scipp/scipp/pull/3070>`_.

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
Jan-Lukas Wynen :sup:`a`\ ,
and Sunyoung Yoo :sup:`a`


v23.01.0
--------

Features
~~~~~~~~

* Added new data structure :class:`scipp.DataGroup`, a more flexible version of ``Dataset`` that does not enforce alignment and supports nesting.
* Added support for arbitrary unit, degrees Celsius and other special units `#2931 <https://github.com/scipp/scipp/pull/2931>`_.
* :class:`scipp.Dataset` now supports ``drop_coords``, which returns :class:`scipp.Dataset` without the given coordinate by names  `#2940 <https://github.com/scipp/scipp/pull/2940>`_.
* :class:`scipp.DataArray` now supports ``drop_coords``/ ``drop_masks``/ ``drop_attrs``, which returns :class:`scipp.DataArray` without the given coordinates/masks/attributes names by names `#2940 <https://github.com/scipp/scipp/pull/2940>`_.
* Added ``clear``, ``copy`` and ``popitem`` to the ``.coords``, ``.masks``, and ``.attrs`` accessors `#3014 <https://github.com/scipp/scipp/pull/3014>`_.

Breaking changes
~~~~~~~~~~~~~~~~
* The HDF5 format changed to accommodate more units, old files can still be loaded but new ones cannot be loaded with an old version of Scipp `#2931 <https://github.com/scipp/scipp/pull/2931>`_.
* Implicit and explicit broadcasting of operands with variances in operations was disabled.
  This introduces correlations that Scipp cannot handle and would therefore silently underestimate uncertainties.
  Instead a :py:class:`scipp.VariancesError` is raised now `#2895 <https://github.com/scipp/scipp/pull/2895>`_.
* The ``.value`` and ``.variance`` properties now return `numpy scalars <https://numpy.org/doc/stable/reference/arrays.scalars.html>`_ for numeric types.
  This will not affect the majority of use cases but may break some edge cases, in particular ``isinstance`` checks.
  `#2962 <https://github.com/scipp/scipp/pull/2962>`_

Bugfixes
~~~~~~~~

* Fix a bug in :py:func:`scipp.hist` and :py:func:`scipp.bin`, leading to assignment of records with very large coord values outside the bin boundaries to a bin `#2923 <https://github.com/scipp/scipp/pull/2923>`_.
* Fix a bug in open end slicing of bins, in case the given right/left end is smaller/bigger than the min/max value of the slicing target, it uses the given end instead of min/max for the open end `#2933 <https://github.com/scipp/scipp/pull/2933>`_.
* Fix issue with events close to upper or lower bin bounds getting dropped by :func:`scipp.lookup` with edges that form a "linspace" `#2942 <https://github.com/scipp/scipp/pull/2942>`_.
* Fix minor issue with events close to bin bounds getting assigned to the wrong bin by :func:`scipp.lookup` with edges that form a "linspace" `#2942 <https://github.com/scipp/scipp/pull/2942>`_.
* Fix two memory usage and performance issue that affected certain cases of :py:func:`scipp.hist` `#2977 <https://github.com/scipp/scipp/pull/2977>`_.
* Fix bounds check in integer-array indexing, which previously silently skipped out-of-range indices `#2986 <https://github.com/scipp/scipp/pull/2986>`_.

Documentation
~~~~~~~~~~~~~

Deprecations
~~~~~~~~~~~~

Stability, Maintainability, and Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* Added type check of masks during binary operations of :py:class:`scipp.DataArray` to provide easier error messages.

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
Jan-Lukas Wynen :sup:`a`\ ,
and Sunyoung Yoo :sup:`a`

v22.11.0 (November 2022)
------------------------

Features
~~~~~~~~

* Added :py:func:`scipp.elemwise_func` for defining custom transformations of input variables based on a Python function compiled via ``numba.cfunc`` `#2886 <https://github.com/scipp/scipp/pull/2886>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* The SciPy wrappers ``integrate``, ``interpolate``, ``ndimage``, ``optimize``, and ``signal`` were moved into the :py:mod:`scipp.scipy` submodule `#2881 <https://github.com/scipp/scipp/pull/2881>`_.

Deprecations
~~~~~~~~~~~~

* Scipp is migrating to use `Plopp <https://scipp.github.io/plopp>`_ instead of a built-in plotting solution.
  The latter is now deprecated.
  Scipp v23.03.0 (March 2023) and all later versions will use Plopp by default.
  The built-in plotting solution is deprecated and will be removed (at the earliest) in Scipp v23.08.0 (August 2023) without further warning.

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
and Jan-Lukas Wynen :sup:`a`

v0.17.0 (October 2022)
----------------------

Features
~~~~~~~~

* :class:`scipp.Bins` now supports ``__getitem__``, providing a shorthand for extracting events based on a coord value or value interval. This is equivalent functionality to label-based indexing for dense data but considers the coordinates of the underlying bin content `#2831 <https://github.com/scipp/scipp/pull/2831>`_.
* Much faster ``obj.bins.concat`` operations in presence of many bins `#2825 <https://github.com/scipp/scipp/pull/2825>`_.
* When mapping between many input and output bins:
  Avoid huge memory use and slow performance in use of :py:func:`scipp.bin` and :py:func:`scipp.group` when only entire bins are combined (rather than splitting bins based on bin content coordinates).
  This avoids a number of cases where Python kernel used to crash since Scipp ran out of memory `#2827 <https://github.com/scipp/scipp/pull/2827>`_.
* :py:func:`scipp.sum` now also accepts a sequence of dimension labels `#2827 <https://github.com/scipp/scipp/pull/2827>`_.
* Added aliases for units to customize string formatting and support user defined units `#2855 <https://github.com/scipp/scipp/pull/2855>`_.
* Add ``pip`` (PyPI) packages for Python 3.11.

Breaking changes
~~~~~~~~~~~~~~~~

* Changed how units are stored in HDF5 files. Old files can still be loaded but new files cannot be loaded by an old scipp `#2836 <https://github.com/scipp/scipp/pull/2836>`_.
* Removed support for custom units as implemented by LLNL/Units. They were only usable because of an oversight `#2843 <https://github.com/scipp/scipp/pull/2843>`_.

Bugfixes
~~~~~~~~

* Fix an issue in the setup of automatic bin edges when giving a bin count to :py:func:`scipp.bin`, :py:func:`scipp.hist`, and :py:func:`scipp.rebin`. This lead to a minuscule shift in the resulting edges, which is irrelevant in most cases, but results in events at the lower bin bound to be placed in a correct but lower bin than expected `#2838 <https://github.com/scipp/scipp/pull/2838>`_.
* Fix runtime configuration not loading a file int he working directory `#2848 <https://github.com/scipp/scipp/pull/2848>`_.
* Fix bug in py:func:`scipp.compat.from_pandas` with non-string names `#2851 <https://github.com/scipp/scipp/pull/2851>`_.

Documentation
~~~~~~~~~~~~~

Deprecations
~~~~~~~~~~~~

- The ``concat`` action of :py:func:`scipp.groupby` is deprecated. Use :py:func:`scipp.group` and :py:func:`scipp.bin` instead `#2827 <https://github.com/scipp/scipp/pull/2827>`_.

Stability, Maintainability, and Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Gregory Tucker :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
and Jan-Lukas Wynen :sup:`a`


v0.16.4 (September 2022)
------------------------

Bugfixes
~~~~~~~~

* Fix missing support for comparison for spatial dtypes such as ``affine_transform3`` `#2821 <https://github.com/scipp/scipp/pull/2821>`_.
* Fix :func:`scipp.spatial.rotations` to support creation of quaternion arrays with more than 1 dimension `#2822 <https://github.com/scipp/scipp/pull/2822>`_.

v0.16.2 (August 2022)
---------------------

Bugfixes
~~~~~~~~

* Fix for ``pythreejs-2.4.0``.
* Fix ipython display for ``ipywidgets>=8``

v0.16.0 (August 2022)
---------------------

Features
~~~~~~~~

* :meth:`scipp.Bins.concat` now supports concatenation of all dims, with ``da.bins.concat()`` `#2726 <https://github.com/scipp/scipp/pull/2726>`_.
* Added support for calling :func:`scipp.arange` with strings to create datetime ranges `#2729 <https://github.com/scipp/scipp/pull/2729>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* The plots are no longer interactive by default, standard Matplotlib rules now apply (the inline static backend is the default). Use ``%matplotlib widget`` for interactive plots `#2671 <https://github.com/scipp/scipp/pull/2671>`_.
* The ``copy`` method of groupby has been removed. Use advanced indexing instead `#2676 <https://github.com/scipp/scipp/pull/2676>`_.

Bugfixes
~~~~~~~~

* Binary arithmetic operations such as ``x + x`` of ``x * x``, i.e., with both operands the same, now handle correlations correctly and result in the correct variances in the output `#2709 <https://github.com/scipp/scipp/pull/2709>`_.
* Made ``__sizeof__`` and related functions more accurate `#2705 <https://github.com/scipp/scipp/pull/2705>`_.
* 1-D entries with identical dimensions but different coordinate units are no longer plotted on the same axes `#2728 <https://github.com/scipp/scipp/pull/2728>`_.
* Fixed size estimates for slices of binned variables `#2719 <https://github.com/scipp/scipp/pull/2719>`_.
* ``Coords`` and other dict-like types now raise exceptions when modifying during iteration instead of causing a segfault `#2719 <https://github.com/scipp/scipp/pull/2719>`_.
* Data arrays with missing coords can now be plotted using the experimental plotting `#2748 <https://github.com/scipp/scipp/pull/2748>`_.
* Fix a bug in the argument detection when using setitem in combination with value-based slicing `#2750 <https://github.com/scipp/scipp/pull/2750>`_.
* Fixed broken profile plot on 3-D data `#2746 <https://github.com/scipp/scipp/pull/2746>`_.
* Fixed broken boolean-indexing and sort operations on arrays that contain binned data `#2759 <https://github.com/scipp/scipp/pull/2759>`_.
* ``transform_coords`` now works with datasets with only coordinates but no data `#2755 <https://github.com/scipp/scipp/pull/2755>`_.
* Fixed a bug when using :func:`scipp.hist` on a Dataset that contains binned data `#2764 <https://github.com/scipp/scipp/pull/2764>`_.
* Fixed the ``rename`` methods to also rename bin coords and attrs of binned data `#2774 <https://github.com/scipp/scipp/pull/2774>`_.
* Fixed the ``rename_dims`` methods which failed to check for conflicting bin-edge coords of length 2 failling outside the object's dimensions `#2775 <https://github.com/scipp/scipp/pull/2775>`_.

Performance
~~~~~~~~~~~

- Advanced indexing is now much faster.
- Sorting is now much faster.

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
and Jan-Lukas Wynen :sup:`a`

v0.15.0 (July 2022)
-------------------

Features
~~~~~~~~

* Added conversion from Scipp objects to Xarray objects (previously only converting from Xarray to Scipp was available) `#2624 <https://github.com/scipp/scipp/pull/2624>`_.
* Added option for a more compact string format of variables `#2625 <https://github.com/scipp/scipp/pull/2625>`_.
* Overhauled and streamlined usability of functions related to binning, grouping, and histogramming with additional features such as automatic bin edges and histogramming ignoring NaN values.
  See :func:`scipp.bin`, :func:`scipp.group`, :func:`scipp.hist`, :func:`scipp.nanhist`, and :func:`scipp.rebin` `#2633 <https://github.com/scipp/scipp/pull/2633>`_.
* Added support for histogramming and binning variables, in addition to existing support for data arrays `#2678 <https://github.com/scipp/scipp/pull/2678>`_.
* Added keyword argument syntax to :func:`scipp.transform_coords` for more concise user experience in simple single-step transformations `#2670 <https://github.com/scipp/scipp/pull/2670>`_.
* Generalized :func:`scipp.lookup` to also support non-histogram functions for value lookup, with supported modes "previous" and "nearest".
  Also adding support for custom fill values `#2681 <https://github.com/scipp/scipp/pull/2681>`_.
* Added possibility to pass keyword arguments to ``rename_dims``, matching the signature of ``rename`` `#2689 <https://github.com/scipp/scipp/pull/2689>`_.
* Python's builtin conversion to "bool" using ``__bool__`` is now supported for 0-D boolean variables.
  This makes the results of comparison usable with ``assert`` or ``if`` `#2695 <https://github.com/scipp/scipp/pull/2695>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* :func:`scipp.optimize.curve_fit` now raises ``ValueError`` if the input variance contains a 0 `#2620 <https://github.com/scipp/scipp/pull/2620>`_.
* Replaced the widget-based :func:`scipp.table` viewer with a simpler pure-HTML table `#2613 <https://github.com/scipp/scipp/pull/2613>`_.
* :func:`scipp.flatten` now drops mismatching bin edge coordinates instead of raising a ``BinEdgeError`` `#2652 <https://github.com/scipp/scipp/pull/2652>`_.
* The ``targets`` argument of :func:`scipp.transform_coords` is not position-only `#2670 <https://github.com/scipp/scipp/pull/2670>`_.
* :func:`scipp.lookup` now uses NaN as fill value for floating-point valued functions (for points out of bounds or masked points).
   Integral-valued functions are unchanged (using 0 as before).
   See also new argument for custom fill values `#2681 <https://github.com/scipp/scipp/pull/2681>`_.

Bugfixes
~~~~~~~~

* Fix bug where coordinates in were dropped by :func:`scipp.transform_coords` with ``keep_*=False`` even through they were specified as ``targets`` `#2642 <https://github.com/scipp/scipp/pull/2642>`_.
* Fix segmentation fault in :func:`scipp.bin` when binning in 2 or more dimensions with certain coordinate values `#2644 <https://github.com/scipp/scipp/pull/2644>`_.
* Fix issue with events close to upper or lower bin bounds getting dropped by :func:`scipp.bin` and :func:`scipp.histogram` when binning with edges that form a "linspace" `#2644 <https://github.com/scipp/scipp/pull/2644>`_.
* Fix minor issue with events close to bin bounds getting assigned to the wrong bin by :func:`scipp.bin` and :func:`scipp.histogram` when binning with edges that form a "linspace" `#2644 <https://github.com/scipp/scipp/pull/2644>`_.
* Fix issues with copying binned structured data or fields of binned structured data `#2650 <https://github.com/scipp/scipp/pull/2650>`_.
* Fix various missing dimension and/or shape checks when slicing with a condition variable `#2657 <https://github.com/scipp/scipp/pull/2657>`_.
* Fix serious bug in :func:`scipp.bin` that was introduced in scipp-0.12.0. This corrupts data when binning into more than 65536 at a time `#2680 <https://github.com/scipp/scipp/pull/2680>`_.
* Fix issue in :func:`scipp.lookup`, which led to ignored masks in case of integral function values with non-linspace coordinates `#2681 <https://github.com/scipp/scipp/pull/2681>`_.
* Fix reduction operations of 0-D binned data, which previously returned the input unchanged `#2685 <https://github.com/scipp/scipp/pull/2685>`_.

Documentation
~~~~~~~~~~~~~

* Remove two ancient tutorials.

Deprecations
~~~~~~~~~~~~

* The ``histogram`` function is deprecated. The new :py:func:`scipp.hist` can replace it in most cases, otherwise :py:func:`scipp.binning.make_histogrammed` provides the old functionality `#2633 <https://github.com/scipp/scipp/pull/2633>`_.
* The ``bins``, ``edges``, and ``erase`` keyword arguments of :py:func:`scipp.bin` are deprecated. :py:func:`scipp.bin` and :py:func:`scipp.group` can replace it in most cases, otherwise :py:func:`scipp.binning.make_binned` provides the old functionality `#2633 <https://github.com/scipp/scipp/pull/2633>`_.

Stability, Maintainability, and Testing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Gregory Tucker :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
and Jan-Lukas Wynen :sup:`a`

v0.14.0 (May 2022)
------------------

Features
~~~~~~~~

* Added argument ``max_rows`` to :func:`scipp.table` `#2526 <https://github.com/scipp/scipp/pull/2526>`_.
* Added support for converting scalars to builtin objects via :func:`int` and :func:`float` `#2529 <https://github.com/scipp/scipp/pull/2529>`_.
* Reduced time of initial import of Scipp by delaying imports of optional dependencies `#2535 <https://github.com/scipp/scipp/pull/2535>`_.
* Added an ``update`` method to :class:`Coords`, :class:`Attrs`, :class:`Masks`, and :class:`Dataset`  `#2558 <https://github.com/scipp/scipp/pull/2558>`_.
* Support ``dtype=vector3`` in :func:`scipp.isinf` and :func:`scipp.isfinite` `#2593 <https://github.com/scipp/scipp/pull/2593>`_.
* Added support for passing any dict-like objects or iterables of tuples as ``coords``, ``attrs``, and ``masks`` arguments to initializers of :class:`scipp.DataArray` and :class:`scipp.Dataset` `#2603 <https://github.com/scipp/scipp/pull/2603>`_.
* Added support for passing any dict-like objects or iterables of tuples as the ``data`` argument to the initializer of :class:`scipp.Dataset` `#2603 <https://github.com/scipp/scipp/pull/2603>`_.
* Added support for ``DataArray`` and ``Dataset`` in more reduction operations (e.g. :func:`scipp.max`) `#2600 <https://github.com/scipp/scipp/pull/2600>`_.
* Added support for masks in reductions of binned data `#2608 <https://github.com/scipp/scipp/pull/2608>`_.
* Added more reduction operations to the ``.bins`` property (e.g. :func:`scipp.Bins.max`) `#2608 <https://github.com/scipp/scipp/pull/2608>`_.
* Reduce effect of rounding errors when converting units `#2607 <https://github.com/scipp/scipp/pull/2607>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* Changed :meth:`scipp.Variable.dims` and :meth:`scipp.Variable.shape` and corresponding properties in ``DataArray`` and ``Dataset`` to return tuples `2543 <https://github.com/scipp/scipp/pull/2543>`_.
* :func:`scipp.scalar` and :func:`scipp.index` now require keyword arguments for all but the ``value`` argument and :func:`scipp.vector` can take ``value`` positionally `2585 <https://github.com/scipp/scipp/pull/2585>`_.
* Removed ``out`` argument from reduction operations (e.g. :func:`scipp.sum`) `2591 <https://github.com/scipp/scipp/pull/2591>`_.
* :func:`scipp.datetime` and :func:`scipp.datetimes` now raise an error if a datetime string with timezone information is provided.
  Previously this was a ``DeprecationWarning`` from NumPy `2604 <https://github.com/scipp/scipp/pull/2604>`_.
* Replaced the widget-based :func:`scipp.table` viewer with a simpler pure-HTML table `#2613 <https://github.com/scipp/scipp/pull/2613>`_.

Bugfixes
~~~~~~~~

* Fix :func:`scipp.isclose` and :func:`scipp.allclose` to support arguments without a unit `#2528 <https://github.com/scipp/scipp/pull/2528>`_.
* :func:`scipp.to_unit` avoids a rounding problem when converting datetimes. This previously led to errors, e.g., of about 300 nanoseconds when converting a current (2020s) datetime64 from seconds to nanoseconds `#2533 <https://github.com/scipp/scipp/pull/2533>`_.
* Fix handling of keyword arguments in :func:`scipp.optimize.curve_fit`. They were previously checked for conflicts but otherwise ignored `#2545 <https://github.com/scipp/scipp/pull/2545>`_.
* Fix a segmentation fault in :func:`scipp.bin` when grouping by a variable with 0 elements `#2590 <https://github.com/scipp/scipp/pull/2590>`_.
* Fix :func:`scipp.nanmean` for ``dtype=vector3``.
  Previously this did not ignore NaN elements as it should.
  :func:`scipp.isnan` is affected by the same fix.
  Previously it always returned ``False`` for ``dtype=vector3`` `#2593 <https://github.com/scipp/scipp/pull/2593>`_.

Documentation
~~~~~~~~~~~~~

* Added new tutorial: *RHESSI Solar Flares* `2536 <https://github.com/scipp/scipp/pull/2536>`_.
* Added new tutorial: *From tabular data to binned data* `2580 <https://github.com/scipp/scipp/pull/2580>`_.
* Added documentation of our docstring format `2546 <https://github.com/scipp/scipp/pull/2546>`_.
* Configured docs builds to enable plots generated by docstrings and added a custom extension to automate usage of the matplotlib plot directive `2609 <https://github.com/scipp/scipp/pull/2609>`_.

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
and Jan-Lukas Wynen :sup:`a`

v0.13.1 (April 2022)
--------------------

Bugfixes
~~~~~~~~

* Fix exception in :func:`scipp.islinspace` for dtypes other than float64 `#2523 <https://github.com/scipp/scipp/pull/2523>`_.

v0.13.0 (March 2022)
--------------------

Features
~~~~~~~~

* Added ``is_edges`` method to ``coords``, ``attrs``, ``masks``, and ``meta`` properties of DataArray and Dataset `#2469 <https://github.com/scipp/scipp/pull/2469>`_.
* Added support for arithmetic of :class:`scipp.DataArray` and :class:`scipp.Dataset` with builtin ``int`` and ``float`` `#2473 <https://github.com/scipp/scipp/pull/2473>`_.
* Added advanced indexing support: integer array indexing `#2502 <https://github.com/scipp/scipp/pull/2502>`_.
* Added advanced indexing support: boolean variable indexing `#2477 <https://github.com/scipp/scipp/pull/2477>`_.
* Added or exposed to Python more functions for boolean logic `#2480 <https://github.com/scipp/scipp/pull/2480>`_, `#2489 <https://github.com/scipp/scipp/pull/2489>`_.
* ``broadcast`` now supports :class:`scipp.DataArray`, and the ``sizes`` argument `#2488 <https://github.com/scipp/scipp/pull/2488>`_.
* The output of :func:`squeeze` is no longer read-only `2494 <https://github.com/scipp/scipp/pull/2494>`_.
* Added support for spatial dtypes to :func:`scipp.to_unit` and :func:`scipp.isclose` `#2498 <https://github.com/scipp/scipp/pull/2498>`_.

Breaking changes
~~~~~~~~~~~~~~~~

* Changed format of Scipp native HDF5 files to allow for UTF-8 characters in coord names; migration script: ``tools/migration/scipp-0.13-hdf5-files.py`` `#2463 <https://github.com/scipp/scipp/pull/2463>`_.

Bugfixes
~~~~~~~~

* Fix permissions error for configuration file where users do not have write access to their config dir `#2482 <https://github.com/scipp/scipp/pull/2482>`_.
* Fix plotting of data with multi-dimension bin-edge coordinate in the presence of a mask that does not depend on the bin-edge dimension `#2505 <https://github.com/scipp/scipp/pull/2505>`_.
* Fix incorrect axis labels in plots with datetime tick labels `#2507 <https://github.com/scipp/scipp/pull/2507>`_.
* Fix segfault when calling :func:`scipp.GroupByDataArray.copy` or :func:`scipp.GroupByDataset.copy` with out of bounds index `#2511 <https://github.com/scipp/scipp/pull/2511>`_.

Contributors
~~~~~~~~~~~~

Simon Heybrock :sup:`a`\ ,
Neil Vaytet :sup:`a`\ ,
and Jan-Lukas Wynen :sup:`a`

v0.12.1 (February 2022)
-----------------------

Bugfixes
~~~~~~~~

* Fix a plotting bug when 2d data has masks and a coord with no unit `#2470 <https://github.com/scipp/scipp/pull/2470>`_.

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

* ``sc.matrix`` and ``sc.matrices`` have been deprecated in favour of :py:func:`scipp.spatial.linear_transform` and :py:func:`scipp.spatial.linear_transforms`, and will be removed in a future release of Scipp.
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
* Add ``from_pandas`` and ``from_xarray`` for conversion of pandas dataframes, xarray data arrays and dataset to Scipp objects `#2054 <https://github.com/scipp/scipp/pull/2054>`_.
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
* Add rounding functions ``floor``, ``ceil``, and ``round`` that performs similarly to NumPy equivalents `#2147 <https://github.com/scipp/scipp/pull/2147>`_.

Breaking changes
~~~~~~~~~~~~~~~~

.. include:: <isonum.txt>

* Changed names of arithmetic functions to match NumPy's names: ``plus`` |rarr| :func:`scipp.add`, ``minus`` |rarr| :func:`scipp.subtract`, ``times`` |rarr| :func:`scipp.multiply` `#1999 <https://github.com/scipp/scipp/pull/1999>`_.
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
  * Direct creation and initialization of 2-D (or higher) arrays of matrices and vectors is now possible from NumPy arrays.
  * Fix extremely slow initialization of array of vectors or matrices from NumPy arrays.
  * The ``values`` property now returns a NumPy array with ``ndim+1`` (vectors) or ``ndim+2``` (matrices) axes, with the inner 1 (vectors) or 2 (matrices) axes corresponding o the vector or matrix axes.
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
* Add ``ones`` and ``empty`` creation functions, similar to what is known from NumPy `#1732 <https://github.com/scipp/scipp/pull/1732>`_.
* ``scipp.neutron`` has been removed and is replaced by `scippneutron <https://scipp.github.io/scippneutron>`_
* ``scipp.neutron`` (now ``scippneutron``)

  * Support unit conversion to energy transfer, for inelastic TOF experiments `#1635 <https://github.com/scipp/scipp/pull/1635>`_.
  * Support loading/converting Mantid ``WorkspaceGroup``, this will produce a ``dict`` of data arrays `#1654 <https://github.com/scipp/scipp/pull/1654>`_.
  * Fixes to support loading/converting ``McStasNexus`` files `#1659 <https://github.com/scipp/scipp/pull/1659>`_.
* ``isclose`` added (``is_approx`` removed). Fuzzy data comparison ``isclose`` is an analogue to NumPy's ``isclose``
* ``stddevs`` added `#1762 <https://github.com/scipp/scipp/pull/1762>`_.

* Support for datetime

  * ``sc.dtype.datetime64`` with copy-less casting between NumPy and Scipp where possible. `#1639 <https://github.com/scipp/scipp/pull/1639>`_
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
* Support for saving and loading Scipp data structures to HDF5.
* More functions such as ``nanmean`` for better handling of special values such as ``INF`` and ``NaN``.
* TBB (multi-threading) support for MacOS.
* ``scipp.neutron``

  * Improved instrument view, e.g., with buttons to align camera with an axis.
  * Experiment logs (previously using Mantid's ``Run``) are now represented as native Scipp objects, e.g., as scalar attributes holding a data array representing a time-series such as a temperature log.
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
* You can now inspect the global object list of via the ``repr`` for Scipp showing Datasets, DataArrays and Variables
* Internal cleanup and documentation additions.

Notable bug fixes
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
* :sup:`c`\  Tessella, UK
