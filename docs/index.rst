scipp - Multi-dimensional data arrays with labeled dimensions
=============================================================

**scipp** is heavily inspired by `xarray <https://xarray.pydata.org>`_.
While for many applications xarray is certainly more suitable (and definitely much more matured) than scipp, there is a number of features missing in other situations.
If your use case requires one or several of the items on the following list, using scipp may be worth considering:

- Handling of physical units.
- Propagation of uncertainties.
- Support for histograms, i.e., bin-edge axes, which are by 1 longer than the data extent.
- Support for event data, a particular form of sparse data with 1-D (or N-D) arrays of random-length lists, with very small list entries.
- Written in C++ for better performance (for certain applications), in combination with Python bindings.

Currently scipp is moving from its prototype phase into a more consolidated set of libraries.

Scipp consists of generic core libraries (current **scipp-units** and **scipp-core**), and more specific libraries (such as **scipp-neutron** for neutron-scattering data).


Documentation
=============

.. toctree::
   :caption: Getting Started
   :maxdepth: 2

   getting-started/overview
   getting-started/faq
   getting-started/installation

.. toctree::
   :caption: User Guide
   :maxdepth: 2

   user-guide/data-structures
   user-guide/slicing
   user-guide/operations
   user-guide/sparse-data

.. toctree::
   :caption: Python Reference
   :maxdepth: 2

.. toctree::
   :caption: C++ Reference
   :maxdepth: 2

   cpp-reference/api
   cpp-reference/customizing
   cpp-reference/transform

.. toctree::
   :caption: Additional Modules
   :maxdepth: 2

   additional-modules/scipp-neutron
