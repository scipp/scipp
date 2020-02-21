C++ API for constructing variables
==================================

Logically ``Variable`` consists of:

  - unit that refers to the physical unit (kg, m, s, ...),
  - list of dimensions (X, Y, ToF, ...),
  - shape: the extent of data in every dimension (integer number),
  - values,
  - optionally variances.

The keyword arguments factory function is introduced.
It allows to omit the arguments and pass them in arbitrary order.
The full form is following:

.. code-block:: cpp

    makeVariable<float>(Dims{Dim::X, Dim::Y}, Shape{1, 3}, Unit{kg}, Values{1, 2, 3}, Variances{4, 5, 6});

where ``Dims`` is the wrapper structure for the list of dimensions, ``Shape`` wraps dimensions lengths, ``Values`` and ``Variances`` wrap the arguments for constructing ``element_array``.
``Dims`` and ``Shape`` could be combined in ``Dimensions``.:

.. code-block:: cpp

    makeVariable<float>(Dimensions{{Dim::X, 1}, {Dim::Y, 3}}, Unit{kg}, Values{1, 2, 3}, Variances{4, 5, 6});

The default value for ``Unit`` is ``dimensionless``, omitting ``Shape`` and ``Dims`` or ``Dimensions`` leads to the 0-dimensional variable, let say scalar, which contains the data of one element: ``Dimensions`` default value is an empty list of dimensions.

.. code-block:: cpp

    makeVariable<float>(Values{1}, Variances{2});

Omitting both ``Values`` and ``Variances`` gives the default initialized ``Variable``, that contains ONLY values:

.. code-block:: cpp

    makeVariable<float>(Dimensions{Dim::X, 5});

produces a ``Variable`` with values {0, 0, 0, 0, 0}.

For default initialized both values and variances the following syntax is used:

.. code-block:: cpp

    makeVariable<float>(Dimensions{Dim::X, 5}, Values{}, Variances{});

The resulting ``Variable`` contains values ``{0, 0, 0, 0, 0}`` and variances ``{0, 0, 0, 0, 0}``.

``Variable`` has a constructor which takes the ``DType`` as the first argument and acts the same way as a function described above.

.. code-block:: cpp

    makeVariable<float>(Dimensions{Dim::X, 5}, Values{}, Variances{});

is equal to


.. code-block:: cpp

    Variable(dtype<float>, Dimensions{Dim::X, 5}, Values{}, Variances{});

Use the factory function where it is possible, the usage of the constructor generates a big amount of code, though it is convenient for instance in for Python bindings or other case of runtime dispatching.
