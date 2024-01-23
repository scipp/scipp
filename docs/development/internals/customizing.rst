.. _customizing:

Customizing Scipp
=================

Overview
--------

A number of concepts and components of Scipp can and should be customized to the needs of higher-level libraries or to a particular use case.
At this point we support compile-time customization of:

- The maximum number of dimensions supported by operations with ``Variable`` is configured using the ``NDIM_OP_MAX`` constant.
- New or custom ``dtype`` that can be stored as elements in a ``Variable`` and used with the ``transform`` algorithms.

Source code for Scipp is hosted in a github repository `here <https://github.com/scipp/scipp>`_.

Custom dtypes
-------------

To add a new ``dtype``, use ``INSTANTIATE_VARIABLE``, available when including ``scipp/variable/variable.tcc``.
Note that this ``tcc`` file should *never* be included in a header.

- The instantiation macro call must be placed in the ``scipp::variable`` namespace.
- The first marco argument is the display name of the new ``dtype``, and the identifier used for Python bindings.
- The second macro argument is the C++ type.

To support formatting of variables with the new ``dtype`` it should be registered in ``scipp::variable::formatterRegistry()``.
`variable_instantiate_py_object.cpp <https://github.com/scipp/scipp/blob/main/lib/python/variable_instantiate_py_object.cpp>`_ can be used as a reference.

In addition to these steps, it is currently required to manually add Python bindings in several places.
TODO: Improve or document this process.
