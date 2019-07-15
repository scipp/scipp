.. _sparse-data:

Sparse data
===========

Scipp can handle a certain type of sparse data, i.e., data that cannot directly be represented as a multi-dimensional array.
For applications that rely solely on dense arrays of data this section can safely be ignored.

Scipp supports sparse data in shape of a multi-dimensional array of lists.
This could, e.g., be used to store data from an array of sensors/detectors that are read out independently, with potentially widely varying frequency.

If data has a sparse dimension it is always the innermost dimension of a variable.
Since here we are not dealing with a dense array we cannot set values for all ``X`` from a numpy array.
The recommended approach is to slice out all outer dimensions.
Then the remaining values (for a particluar "X" in this case) are a dense array with a list-like interface:

.. ipython:: python

    import numpy as np
    import scipp as sc
    from scipp import Dim

    var = sc.Variable(dims=[Dim.X, Dim.Y],
                      shape=[4, sc.Dimensions.Sparse])

    var[Dim.X, 0]
    var[Dim.X, 0].values = np.arange(3)
    var[Dim.X, 0].values.append(42)
    var[Dim.X, 0].values.extend(np.ones(3))
    var[Dim.X, 1].values = np.ones(6)
    var[Dim.X, 0].values
    var[Dim.X, 1].values
    var[Dim.X, 2].values

Operations such as slicing the sparse dimension are ill-defined and are not supported:

.. ipython:: python
    :okexcept:

    var[Dim.Y, 0]

Operations between variables or datasets broadcast dense data into sparse dimensions:

.. ipython:: python

    scale = sc.Variable(dims=[Dim.X], values=np.arange(2.0, 6))
    var *= scale
    var[Dim.X, 0].values
    var[Dim.X, 1].values
    var[Dim.X, 2].values

Sparse data in a dataset can be associated with a corresponding sparse coordinate and sparse labels.
These are specific for a particular data item.
The sparse coord shadows the global coordinate when accessed via the ``coords`` property of a data item:

.. ipython:: python

    d = sc.Dataset(
            {'a': sc.Variable(dims=[Dim.X, Dim.Y], shape=[2, sc.Dimensions.Sparse]),
             'b': sc.Variable(dims=[Dim.X, Dim.Y], shape=[2, sc.Dimensions.Sparse])},
             coords={
                 Dim.X: sc.Variable([Dim.X], values=np.arange(2.0)),
                 Dim.Y: sc.Variable([Dim.Y], values=np.arange(3.0))})
    d.set_sparse_coord('a', sc.Variable(dims=[Dim.X, Dim.Y], shape=[2, sc.Dimensions.Sparse]))
    d.coords[Dim.Y]
    d['a'].coords[Dim.Y]
    try:
      d['b'].coords[Dim.Y]
    except IndexError:
      print('Dense coord is meaningless for sparse data, so it is also hidden')

The lengths of the sublists between coordinate and values (and variances) must match.
Scipp does not enforce this when modifying sublists, but *does* verify correctness in operations on variables or dataset.
