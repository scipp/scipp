.. _data-structures:

Data Structures (old)
=====================

Dimension labels
----------------

Scipp uses hard-coded labels for dimension naming.
Arbitrary string-based labels are deliberately avoided, for two reasons:

- Matching between coordinates and dimensions based on string-matching is error-prone and could lead to non-obvious bugs and data corruption.
  For example, an operation between two datasets with a mismatching x-coordinate might erroneously succeed if the coordinates of the respective datasets are named ``"x"`` and ``"X"``.
- Potential performance issues in the underlying C++ libraries.

Instead, scipp uses labels such as ``Dim.X``, ``Dim.Y``, or ``Dim.Temperature``.
It is obviously impossible to support an exhaustive list of possible dimension labels.
The intention is to provide different library builds for different science applications, which typically *do* have a well defined set of relevant dimensions.
See :ref:`customizing` on how to do this.

To keep this documentation generic we typically use ``Dim.X`` or ``Dim.Y``, but this should *not* be seen as a recommendation to use these labels for anything but actual positions or offsets in space.

Variable
--------

:py:class:`scipp.Variable` is a labeled multi-dimensional array.
A :py:class:`~scipp.Variable` can be constructed using:

- ``values``: a multi-dimensional array of values, e.g., a :py:class:`numpy.ndarray`
- ``variances``: a (optional) multi-dimensional array of variances for the array values
- ``dims``: a list of dimension labels for each axis of the array
- ``unit``: a (optional) physical unit of the values in the array

Note that variables, unlike :py:class:`xarray.DataArray`, do *not* have coordinate arrays.

.. ipython:: python

    import numpy as np
    import scipp as sc
    from scipp import Dim

    var = sc.Variable(values=np.random.rand(2, 4), dims=[Dim.X, Dim.Y])
    var
    var.unit
    var.values
    try:
      var.variances
    except RuntimeError:
      print('No variances specified, so they do not exist.')

Variances must have the same shape as values, and units are specified using the :py:class:`scipp.units` module:

.. ipython:: python

    var = sc.Variable(values=np.random.rand(2, 4),
                      variances=np.random.rand(2, 4),
                      dims=[Dim.X, Dim.Y],
                      unit=sc.units.m/sc.units.s)
    var
    var.variances

:py:class:`~scipp.Variable` also supports a single *sparse* dimension.
In this case it is currently not possible to set data directly in the constructor.
Instead we create it by specifying a shape:

.. ipython:: python

    var = sc.Variable(dims=[Dim.X, Dim.Y],
                      shape=[4, sc.Dimensions.Sparse],
                      variances=True,
                      unit=sc.units.kg)
    var
    var.shape # The sparse dimension is not part of the shape
    len(var.values[0]) # Initially the extent in the sparse dimension is 0

For more details see :ref:`sparse-data`.

A 0-dimensional variable contains a single value (and an optional variance).
For convenience, singular versions of the ``values`` and ``variances`` properties are provided:

.. ipython:: python

    var_0d = sc.Variable(variances=True, unit=sc.units.kg)
    var_0d
    var_0d.value = 2.3
    var_0d.variance

An exception is raised from the ``value`` and ``variance`` properties if the variable is not 0-dimensional.
Note that a variable with one or more dimension extent(s) of 1 contains just a single value as well, but the ``value`` property will nevertheless raise an exception.

.. _data-structures-dataset:

Dataset
-------

:py:class:`scipp.Dataset` is a dict-like container of labeled arrays.
Individual data items are accessed using a string as a dict key.
A data item is essentially a :py:class:`~scipp.Variable` object together with relevant coords and labels.

In addition to data items, a dataset has the following key properties:

- ``coords``: a dict-like container of coordinates for the dataset, accessed using a dimension label as dict key.
- ``labels``: a dict-like container of labels (auxiliary coordinates) for the dataset, accessed using a string as dict key.
- ``attrs``: a dict-like container of attributes for the dataset, accessed using a string as dict key.

Note that ``coords`` in scipp correspond to dimension-coordinates in xarray, whereas ``labels`` corresponds to non-dimension coordinates.
See also the `xarray documentation <http://xarray.pydata.org/en/stable/data-structures.html#coordinates>`_.

The key distinction between ``coords``, ``labels``, and ``attrs`` is that the former two are required to match in operations between multiple datasets whereas the latter one is not.
All three are internally a :py:class:`~scipp.Variable`, i.e., they have a physical unit and optionally variances.

.. ipython:: python

    d = sc.Dataset(
            {'a': sc.Variable(dims=[Dim.X, Dim.Y], values=np.random.rand(2, 3)),
             'b': sc.Variable(1.0)},
             coords={
                 Dim.X: sc.Variable([Dim.X], values=np.arange(2.0), unit=sc.units.m),
                 Dim.Y: sc.Variable([Dim.Y], values=np.arange(3.0), unit=sc.units.m)},
             labels={
                 'aux': sc.Variable([Dim.Y], values=np.random.rand(3))})
    d
    d.coords[Dim.X].values
    d['a']
    d['a'].values
    d['a'].has_variances

All variables in a dataset must have consistent dimensions.
Thanks to labeled dimensions transposed data is supported:

.. ipython:: python

    d['c'] = sc.Variable(dims=[Dim.Y, Dim.X], values=np.random.rand(3, 2))
    d

The name of a data item serves as a dict key and the usual methods are available:

.. ipython:: python

    for name, data in d:
        print(name)
    'a' in d
    'x' in d
