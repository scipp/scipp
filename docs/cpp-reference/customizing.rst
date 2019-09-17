.. _customizing:

Customizing Scipp
=================

Overview
--------

A number of concepts and components of **scipp** can and should be customized to the needs of higher-level libraries or to a particular use case.
At this point we support compile-time customization of:

- Available units and unit combinations
- Dimension labels
- The underlying container used for sparse data.
- New or custom types that can be stored as elements in a ``Variable``.
- The built-in types that can be support in operations and by the ``transform`` algorithms.

Some of these are adaptable more readily than others, which require more in-depth changes.

Source code for scipp is hosted in a github repository `here <https://github.com/scipp/scipp>`_.

Units systems and dimension labels
----------------------------------

It is possible to select an existing system or to add a new one.

Selecting an existing system
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ``cmake`` flag ``SCIPP_UNITS_SYSTEM`` is used to select the system of units (and dimension labels).
Currently the following options are available:

- ``Dummy``.
  For unit-testing only.
  This is incomplete and not useful in practice.
- ``Neutron``.
  The system used in ``scipp-neutron``.
  This contains some SI units as well as a unit for "counts" and some particle-physics-related units like ``meV`` and ``meV/c``.

Creating a new system
~~~~~~~~~~~~~~~~~~~~~

1. Create files following the same pattern as ``units/include/scipp/units/dummy.h`` and ``units/dummy.cpp``.
   Adapt the namespace name in both header and source and rename the ``SCIPP_UNITS_DUMMY`` macro.
   This macro is used for inlining the namespace, enabling ``Variable`` and the rest of ``scipp-core`` to pick up the selected unit automatically.
2. In the header file:

   a. Adapt the ``supported_units`` helper.
      This must list all units and unit combinations that are to be supported (this is a shortcoming of the current implementation, which is based on the compile-time units library ``boost-units``).
      For an example of how more complex (heterogeneous) systems can be created see ``units/include/scipp/units/neutron.h``.
   b. Adapt the ``counts_unit`` helper.
      This is used to define the unit for "counts", e.g., the number of detector neutrons, or measurements with a certain value.
      When working with sparse data that is histogrammed later (or data that is a histogram in the first place) it is recommended to use something other than ``dimensionless`` here.
      Otherwise operations like ``rebin`` cannot correctly distinguish data that represents counts or densities of counts from other data.
   c. Adapt the dimensions labels in the call to the macro ``SCIPP_UNITS_DECLARE_DIMENSIONS``.
3. Include the new header in ``units/include/scipp/units/unit.h``.
4. Adapt the documentation of ``SCIPP_UNITS_SYSTEM`` in ``units/CMakeLists.txt``.

To select the new system during compilation, use the ``cmake`` flag ``-DSCIPP_UNITS_SYSTEM=NewSystem``.
This will define the macro ``SCIPP_UNITS_NEWSYSTEM``.

Container used for sparse data
------------------------------

This is currently defined in the header ``core/include/scipp/core/variable.h`` with the helper ``sparse_container``.
This has to be set to a vector-like type, such as ``std::vector`` or ``boost::container::small_vector``.
In the latter case the small-size of the vector can be used to balance performance/locality against memory overhead.

Custom types
------------

To do.
See https://github.com/scipp/scipp/issues/265.

Built-in types
--------------

The helper ``VariableConceptHandle`` in the header ``core/include/scipp/core/variable.h`` is used to list all types that can be supported in calls to ``transform`` (and its in-place variants) for ``Variable``.
Essentially adding a type here will add another alternative to the variant in ``Variable`` that is used to hold data.

**Note**

It is *not* required to add all types here.
``Variable`` can also hold arbitrary other types, as long as the relevant code has been instantiated.
What is described above is only relevant if a higher level of integration (beyond simply holding data and supporting slicing) is required.
