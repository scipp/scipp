.. _dtype:

Data types
==========

Scipp supports the most common data types (``dtype``).
In most cases the ``dtype`` is derived from the data, e.g., when passing a :py:class:`numpy.ndarray` to scipp, scipp will use ``dtype`` provided by numpy:

.. ipython:: python

    import scipp as sc
    from scipp import Dim

    var = sc.Variable(dims=[sc.Dim.X], values=np.arange(4.0))
    var.dtype    
    var = sc.Variable(dims=[sc.Dim.X], values=np.arange(4))
    var.dtype    

The ``dtype`` may also be scecified using a keyword argument, both :py:class:`numpy.dtype` and :py:class:`scipp.dtype` are supported:

.. ipython:: python

    var = sc.Variable(dims=[Dim.X], shape=[2], dtype=sc.dtype.float)
    var.dtype

    var = sc.Variable(dims=[Dim.X], shape=[2], dtype=np.dtype(np.float32))
    var.dtype

If no data is provided the default is ``double``:

.. ipython:: python

    var = sc.Variable()
    var.dtype    

:ref:`customizing` explains how support for additional types can be added.
