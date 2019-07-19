.. _slicing:

Slicing
=======

Data in a :py:class:`~scipp.Variable` or :py:class:`~scipp.Dataset` can be indexed in a similar manner to NumPy and xarray.
The dimension to be sliced is specified using a dimension label and, in contrast to NumPy, positional dimension lookup is not available.
Positional indexing with an integer or an integer range is implemented by the ``slice`` method, which is available for variables, datasets, as well as items of a dataset.
In all cases a *view* is returned, i.e., just like when slicing a :py:class:`numpy.ndarray` no copy is performed.

.. ipython:: python

    import numpy as np
    import scipp as sc
    from scipp import Dim

    d = sc.Dataset(
            {'a': sc.Variable(dims=[Dim.X, Dim.Y], values=np.random.rand(2, 3)),
             'b': sc.Variable(dims=[Dim.Y, Dim.X], values=np.random.rand(3, 2)),
             'c': sc.Variable(dims=[Dim.X], values=np.random.rand(2)),
             'd': sc.Variable(1.0)},
             coords={
                 Dim.X: sc.Variable([Dim.X], values=np.arange(2.0), unit=sc.units.m),
                 Dim.Y: sc.Variable([Dim.Y], values=np.arange(3.0), unit=sc.units.m)})

    d[Dim.X, 1]

As when slicing a :py:class:`numpy.ndarray`, the dimension ``Dim.X`` is removed since no range is specified.
When slicing a dataset a number of other things happen as well:

- The coordinate for ``Dim.X`` is removed.
- Any data item that does not depend on ``Dim.X`` (``d`` in this case) is removed.

.. ipython:: python

    d[Dim.X, 1:2]

When slicing with a range:

- The coordinate is *not* removed.
  As above, this is the same behavior as in numpy.
- Any data item that does not depend on ``Dim.X`` (``d`` in this case) is removed.

Slicing and item access can be done in arbitrary order with identical results:

.. ipython:: python

    d[Dim.X, 1:2]['a'] == d['a'][Dim.X, 1:2]
    d[Dim.X, 1:2]['a'].coords[Dim.X] == d.coords[Dim.X][Dim.X, 1:2]
