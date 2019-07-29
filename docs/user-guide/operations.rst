.. _operations:

Operations with variables and datasets
======================================

Operations "align" data items based on their dimension labels:

.. ipython:: python

    import numpy as np
    import scipp as sc
    from scipp import Dim

    a = sc.Variable(values=np.random.rand(2, 4),
                    variances=np.random.rand(2, 4),
                    dims=[Dim.X, Dim.Y],
                    unit=sc.units.m)
    b = sc.Variable(values=np.random.rand(4, 2),
                    variances=np.random.rand(4, 2),
                    dims=[Dim.Y, Dim.X],
                    unit=sc.units.s)
    a/b

Note how operations with variables correctly propagate uncertainties (the variances), in contrast to a naive implementation using numpy:

.. ipython:: python

    result = a/b
    result.values
    a.values/np.transpose(b.values)
    result.variances
    a.variances/np.transpose(b.variances)

The implementation assumes uncorrelated data and is otherwise based on, e.g., `Wikipedia: Propagation of uncertainty <https://en.wikipedia.org/wiki/Propagation_of_uncertainty#Example_formulae>`_.
See also :ref:`propagation_of_uncertainties` for the concrete equations used for error propagation.

Missing dimensions in the operands are automatically broadcast:

.. ipython:: python

    a.values
    a -= a[Dim.X, 1]
    a.values

Units are required to be compatible:

.. ipython:: python
    :okexcept:

    a + b

Operations with datasets or items of datasets pair data items based on their names and compare the coordinate values.
If names or coordinates do not match, operations fail.

.. ipython:: python
    :okexcept:

    d = sc.Dataset(
            {'a': sc.Variable(dims=[Dim.X, Dim.Y], values=np.random.rand(2, 3)),
             'b': sc.Variable(dims=[Dim.Y, Dim.X], values=np.random.rand(3, 2)),
             'c': sc.Variable(dims=[Dim.X], values=np.random.rand(2)),
             'd': sc.Variable(1.0)},
             coords={
                 Dim.X: sc.Variable([Dim.X], values=np.arange(2.0), unit=sc.units.m),
                 Dim.Y: sc.Variable([Dim.Y], values=np.arange(3.0), unit=sc.units.m)})
    d['a'] -= d['b'] # transposing
    d['a'] -= d[Dim.X, 1]['b'] # broadcasting
    d['a'] -= d[Dim.X, 1:2]['b'] # fail due to coordinate mismatch

