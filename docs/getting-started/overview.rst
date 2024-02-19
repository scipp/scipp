.. _overview:

What is Scipp?
==============

Scipp is the result of a long evolution of packages for scientific computing in Python.
`NumPy <https://numpy.org/>`_, the fundamental package for scientific computing in Python providing a multi-dimensional array object, sits at the core of this evolution.
On top of this, libraries such as `SciPy <https://scipy.org/>`_, `Pandas <https://pandas.pydata.org/>`_, and `Xarray <https://docs.xarray.dev>`_ provide more advanced functionality and user convenience.

Scipp's central ambition is to make scientific data analysis **safer** and **intuitive**.
Safety, i.e., avoidance, detection, and prevention of mistakes, is crucial given the rapidly growing complexity of scientific data and data analysis.
At the same time it has to be ensured that scientific results are reproducible.
For reproducible results, open data must be complemented by open and non-cryptic analysis scripts.
This enables the user, the referee, or other scientists to re-run the analysis or to inspect and understand each processing step.

Scipp aims to achieve its ambition in multiple ways:

- A concise and intuitive "language" (programming interface) allows the user to clearly express their intent and makes this intent apparent to future readers of the code.
  For example, *named dimensions* remove the need for cryptic axis indices or explicit array transposition and broadcasting.
- Coordinate arrays associated with an array of data ensure that no incompatible data is combined and allow for direct creation of meaningful plots with labeled axes.
- Physical units associated with all arrays as well as their coordinate arrays eliminate a whole set of potential errors.
  This includes the risk of combining data with different units (such as adding "meters" and "seconds") or unit scales (such as adding "meters" and "millimeters").
- *Binned data* provides an efficient and powerful multi-dimensional view of tabular data.
  This unifies the flexibility of working with such record-based data with the same concise, intuitive, and safe interface that Scipp provides for dense arrays.
- Features targeting `Jupyter <https://jupyter.org/>`_, such as rich HTML visualizations of data, give the user the tools to understand their data at every step and share their work with collaborators.

Scipp comes multi-threading enabled by default, providing good out-of-the-box performance for many applications.


What Scipp is not
-----------------

Scipp is not intended for simulations or HPC applications which often require highly specialized and performant code.
Scipp also does not target machine learning applications.


Comparison with other software
------------------------------

- `Pandas <https://pandas.pydata.org/>`_ is a mature and extremely powerful package if you are mainly working with tabular 1-D data.
  Compared with Scipp it lacks support for multi-dimensional labeled data, binned data, and physical units.
- `Xarray <https://docs.xarray.dev>`_ supports multi-dimensional labeled data similar to Scipp, but is considerably more mature.
  Compared with Scipp it lacks support for binned data and physical units.
  Experimental support for physical units using `Pint <https://pint.readthedocs.io>`_ is available with `pint-xarray <https://pint-xarray.readthedocs.io>`_.
  Xarray comes with `Dask <https://www.dask.org/>`_ support for highly scalable parallel computing.
- `Awkward Array <https://awkward-array.org>`_ is a powerful package for nested, variable-sized data.
  Its support for records is similar to Scipp's binned data.
  Compared with Scipp it lacks support for multi-dimensional labeled data and physical units.
  It may be combined with Xarray (and Pint) to achieve this, but support for multi-dimensional binning operations may be lacking.


The data array concept
----------------------

The core data structure of Scipp is :py:class:`scipp.DataArray`.
A data array is essentially a multi-dimensional array with associated dicts of coordinates and masks.
For a more detailed explanation we refer to the `documentation of DataArray <../user-guide/data-structures.rst#DataArray>`_.

Scipp labels dimensions and their associated coordinates with labels such as ``'x'``, ``'temperature'``, or ``'wavelength'``.

Operations between data arrays "align" data items based on their names, dimension labels, and coordinate values.
That is, if names or coordinates do not match, operations fail.
Missing dimensions in the operands are automatically `broadcast <https://numpy.org/doc/stable/user/basics.broadcasting.html>`_.
In general Scipp does not support automatic re-alignment of data.

Data arrays with aligned coordinates can be combined into datasets.
A dataset is essentially a dict-like container of data arrays with common associated coordinates.


Physical units
--------------

Data in a data array as well as its coordinates are associated with physical units.
All operations take the unit into account:

- Operations fail if the units of coordinates do not match.
- Operations such as addition fail if the units of the data items do not match.
- Operations such as multiplication produce an output with a new unit, e.g., :math:`m^{2}` results from multiplication of two inputs with unit :math:`m`.


Scattered data (event data)
---------------------------

Scipp supports event data in form of arrays of variable-length tables.
Conceptually, we distinguish *dense* and *binned* data.

- Dense data is the common case of a regular, e.g., 2-D, grid of data points.
- Binned (event) data (as supported by Scipp) is semi-regular data, i.e., a N-D array/grid of variable-length tables.
  That is, only the internal "dimension" of the tables (event lists) are irregular.

The target application of this is measuring random *events* in an array of detector pixels.
In contrast to a regular image sensor, which may produce a 2-D image at fixed time intervals (which could be handled as 3-D data), each detector pixel will see a different event rate (photons or neutrons) and is "read out" at uncorrelated times.

Scipp handles such event data by supporting *binned data*, which we explicitly distinguish from *histogrammed data*.
Binned data can be converted into histogrammed data by summing over all the events in a bin.


Histograms and bin-edge coordinates
-----------------------------------

Coordinates for one or more of the dimensions in a data array can represent bin edges.
The extent of the coordinate in that dimension exceeds the data extent (in that dimension) by 1.
Bin-edge coordinates frequently arise when we histogram event-based data.
The event data described above can be used to produce such a histogram.

Bin-edge coordinates are handled naturally by operations with datasets.
Most operations on individual data elements are unaffected.
Other operations such as ``concat`` take the edges into account and ensure that the resulting concatenated edge coordinate is well-defined.
There are also a number of operations specific to data with bin-edges, such as ``rebin``.


Variances and propagation of uncertainties
------------------------------------------

Scipp provides basic features for propagation of uncertainties through operations.
Scipp has no way of tracking correlations so this feature has its limitations and may be effectively useless for certain applications.

Data in a data array support optional variances in addition to their values.
The variances are accessed using the ``variances`` property and have the same shape and dtype as the ``values`` array.
All operations take the variances into account:

- Operations fail if the variances of coordinates are not identical, element by element.
  For the future, we are considering supporting inexact matching based on variances of coordinates but currently this is not implemented.
- Operations such as addition or multiplication propagate the errors to the output.
  An overview of the method can be found in `Wikipedia: Propagation of uncertainty <https://en.wikipedia.org/wiki/Propagation_of_uncertainty>`_.
  The implemented mechanism assumes uncorrelated data.
