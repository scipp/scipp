C++ API for constructing ``Variable``
=====================================

Logicaly ``Variable`` consists of:

    - `unit <../doxygen/unit__impl_8h.html>`_ that refers to the physical unit (kg, m, s, ...),
    - list of `dimensions <../doxygen/dimension_8h.html>`_ (X, Y, ToF, ...),
    - shape: the extent of data in every dimension (integer number),
    - values,
    - optionally variances.

The `keyword` arguments fabric function is introduced. It allows to omit the arguments and pass them
in arbitrary order. The full form is following:

.. code-block:: cpp

    makeVariable<float>(Dims{Dim::X, Dim::Y}, Shape{1, 3}, Unit{kg}, Values{1, 2, 3}, Variances{4, 5, 6});

where `Dims` is the wrapper structure for the list of dimensions, `Shape` wraps dimensions lengths,
`Values` and `Variances` wrap the arguments for constructing
`element_array <../doxygen/classscipp_1_1core_1_1detail_1_1element__array.html>`_ . `Dims` and `Shape`
could be combined in `class Dimensions <../doxygen/classscipp_1_1core_1_1Dimensions.html>`_:

.. code-block:: cpp

    makeVariable<float>(Dimensions{{Dim::X, 1}, {Dim::Y, 3}}, Unit{kg}, Values{1, 2, 3}, Variances{4, 5, 6});

The default values for `Unit` is `dimensionless <../doxygen/unit__impl_8h_source.html>`_, omitting
`Shape` and `Dims` or `Dimensions` leads to the 0-dimensional variable, let say scalar, which contains
the data of one element: `Dimensions` default value is an empty list of dimensions.

.. code-block:: cpp

    makeVariable<float>(Values{1}, Variances{2});

Omitting both `Values` and `Variances` gives the default initialized `Variable`, that contains ONLY
values:

.. code-block:: cpp

    makeVariable<float>(Dimensions{Dim::X, 5});

produces `Variable` with values {0, 0, 0, 0, 0}.

For default initialized both values and variances the following syntax is used:

.. code-block:: cpp

    makeVariable<float>(Dimensions{Dim::X, 5}, Values{}, Variances{});

The resulting `Variable` contains values {0, 0, 0, 0, 0} and variances {0, 0, 0, 0, 0}.

`Variable` has the `constructor <../doxygen/classscipp_1_1core_1_1Variable.html>`_ which takes the
`DType <../doxygen/core_2include_2scipp_2core_2dtype_8h.html>`_ as a first argument and acts the same
way as a function described above.

.. code-block:: cpp

    makeVariable<float>(Dimensions{Dim::X, 5}, Values{}, Variances{});

is equal to


.. code-block:: cpp

    Variable(dtype<float>, Dimensions{Dim::X, 5}, Values{}, Variances{});

Use the fabric function where it is possible, the usage of the constructor generates a big amount
of code, though it is convenient for instance in for python bindings or other case of runtime
dispatching.

