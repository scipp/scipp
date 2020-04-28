.. _customizing:

Customizing Scipp
=================

Overview
--------

A number of concepts and components of **scipp** can and should be customized to the needs of higher-level libraries or to a particular use case.
At this point we support compile-time customization of:

- New or custom ``dtype`` that can be stored as elements in a ``Variable`` and by the ``transform`` algorithms.
- Available units and unit combinations
- The underlying container used for event data.

Some of these are adaptable more readily than others, which require more in-depth changes.

Source code for scipp is hosted in a github repository `here <https://github.com/scipp/scipp>`_.

Custom dtypes
-------------

To add a new ``dtype``, use ``INSTANTIATE_VARIABLE``, available when including ``scipp/variable/variable.tcc``.
Note that this ``tcc`` file should *never* be included in a header.

- The instantiation macro call must be placed in the ``scipp::variable`` namespace.
- The first marco argument is the display name of the new ``dtype``, and the identifier used for Python bindings.
- The second macro argument is the C++ type.

To support formatting of variables with the new ``dtype`` it should be registered in ``scipp::variable::formatterRegistry()``.
`variable_instantiate_py_object.cpp <https://github.com/scipp/scipp/blob/master/python/variable_instantiate_py_object.cpp>`_ can be used as a reference.

In addition to these steps, it is currently required to manually add Python bindings in several places.
TODO: Improve or document this process.

Units systems
-------------

The current unit system is based on ``boost::units`` and therefore has a hard-coded set of supported unit combinations.
Customization may therefore be necessary:

1. Adapt the ``supported_units`` helper in ``units/include/scipp/units/unit.h``.
   This must list all units and unit combinations that are to be supported (this is a shortcoming of the current implementation, which is based on the compile-time units library ``boost-units``).
2. Adapt the ``counts_unit`` helper.
   This is used to define the unit for "counts", e.g., the number of detector neutrons, or measurements with a certain value.
   When working with event data that is histogrammed later (or data that is a histogram in the first place) it is recommended to use something other than ``dimensionless`` here.
   Otherwise operations like ``rebin`` cannot correctly distinguish data that represents counts or densities of counts from other data.
3. Currently ``units/include/scipp/units/neutron.h`` provides some additional base units relevant to neutron scattering.
   This contains some SI units as well as a unit for "counts" and some particle-physics-related units like ``meV`` and ``meV/c``.
   More may be defined in a similar way, if SI units are not sufficient.

Container used for event data
------------------------------

This is currently defined in the header ``core/include/scipp/core/variable.h`` with the helper ``event_list``.
This has to be set to a vector-like type, such as ``std::vector`` or ``boost::container::small_vector``.
In the latter case the small-size of the vector can be used to balance performance/locality against memory overhead.
