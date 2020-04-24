.. _overview:

Overview
========

The dataset concept
-------------------

The core data structure of scipp is :py:class:`scipp.Dataset`.
A dataset is essentially a dict-like container of multi-dimensional arrays with common associated coordinates.
Each item in a dataset is accessed with a name string.
For a more detailed explanation we refer to the `documentation of Dataset <../user-guide/data-structures.html#Dataset>`_.

Scipp labels dimensions and their associated coordinates with labels such as ``'x'``, ``'temperature'``, or ``'wavelength'``.

Operations with datasets "align" data items based on their names, dimension labels, and coordinate values.
Missing dimensions in the operands are automatically `broadcast <https://docs.scipy.org/doc/numpy/user/basics.broadcasting.html>`_.
If names or coordinates do not match, operations fail.
In general scipp does not support automatic re-alignment of data.


Physical units
--------------

Data items in a dataset as well as coordinates are associated with a physical unit.
The unit is accessed using the ``unit`` property.
All operations take the unit into account:

- Operations fail if the units of coordinates do not match.
- Operations such as addition fail if the units of the data items do not match.
- Operations such as multiplication produce an output dataset with a new unit for each data item, e.g., :math:`m^{2}` results from multiplication of two data items with unit :math:`m`.


Variances and propagation of uncertainties
------------------------------------------

Data items in a dataset as well as coordinates support optional variances in addition to their values.
The variances are accessed using the ``variances`` property and have the same shape and dtype as the ``values`` array.
All operations take the variances into account:

- Operations fail if the variances of coordinates are not identical, element by element.
  For the future, we are considering supporting inexact matching based on variances of coordinates but currently this is not implemented.
- Operations such as addition or multiplication propagate the errors to the output.
  An overview of the method can be found in `Wikipedia: Propagation of uncertainty <https://en.wikipedia.org/wiki/Propagation_of_uncertainty>`_.
  The implemented mechanism assumes uncorrelated data.


Event data
----------

Scipp supports event data in form of arrays of variable-length lists.
Conceptually, we distinguish *dense* and *event* data.

- Dense data is the common case of a regular, e.g., 2-D, grid of data points.
- Event data (as supported by scipp) is semi-regular data, i.e., a N-D array/grid of random-length lists.
  That is, only the internal "dimension" of the event lists has irregular data distribution.

The target application of this is measuring random *events* in an array of detector pixels.
In contrast to a regular image sensor, which may produce a 2-D image at fixed time intervals (which could be handled as 3-D data), each detector pixel will see a different event rate (photons or neutrons) and is "read out" at uncorrelated times.

Scipp handles such event data by supporting *event coordinates*, i.e., coordinates with a ``dtype`` that is a list:

- The event coordinate depends on all the other dimensions.
  It represents the time-stamps at which event occurred and is thus essentially an array of lists.
- If the coordinate contains events, the corresponding data also contains and must have matching list lengths.
- The values and variances for event data correspond to the event weight and its uncertainty.
  For cases where by definition events all have the *same* weight, scipp supports event weights with a scalar ``dtype`` instead of a list.
  In this case each event-coordinate entry corresponds to a event count given by the scalar weight value for this list.

Events items in a dataset can be seen as a case of unaligned data, with misalignment not just between different items, but also between slices within an item.


Histograms and bin-edge coordinates
-----------------------------------

Coordinates for one or more of the dimensions in a dataset can represent bin edges.
The extent of the coordinate in that dimension exceeds the data extent (in that dimension) by 1.
Bin-edge coordinates frequently arise when we histogram event-based data.
The event data described above can be used to produce such a histogram.

Bin-edge coordinates are handled naturally by operations with datasets.
Most operations on individual data elements are unaffected.
Other operations such as ``concatenate`` take the edges into account and ensure that the resulting concatenated edge coordinate is well-defined.
There are also a number of operations specific to data with bin-edges, such as ``rebin``.

Note that data on bin edges is also supported.
This is currently not in widespread use so there may be limitations we are not aware of.
