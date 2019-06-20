scipp - Multi-dimensional data arrays with labeled dimensions
=============================================================

**scipp** and its Python-counterpart **scippy** are heavily inspired by `xarray <https://xarray.pydata.org>`_.
While for many applications xarray is certainly more suitable (and definitely much more matured) than scipp, there is a number of features missing in other situations.
If your use case requires one or several of the items on the following list, using scipp may be worth considering:

- Handling of physical units.
- Propagation of uncertainties.
- Support for histograms, i.e., bin-edge axes, which are by 1 longer than the data extent.
- Support for event data, a particular form of sparse data with 1-D (or N-D) arrays of random-length lists, with very small list entries.
- Written in C++ for better performance (for certain applications), in combination with Python bindings (**scippy**).

Currently scipp is moving from its prototype phase into a more consolidated set of libraries.

Scipp consists of generic core libraries (current **scipp-units** and **scipp-core**), and more specific libraries (such as **scipp-neutron** for neutron-scattering data).


Documentation
-------------

.. toctree::
   :maxdepth: 2
   :caption: Getting Started

   overview
   faq
   installation

.. toctree::
   :maxdepth: 2
   :caption: User Guide

   data-structures
   slicing

.. toctree::
   :maxdepth: 2
   :caption: Python Reference

.. toctree::
   :maxdepth: 2
   :caption: C++ Reference

   api
   customizing

.. toctree::
   :maxdepth: 2
   :caption: Additional Modules

   scipp-neutron
