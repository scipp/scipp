scipp - Multi-dimensional data arrays with labeled dimensions
=============================================================

**scipp** is heavily inspired by `xarray <https://xarray.pydata.org>`_.
While for many applications xarray is certainly more suitable (and definitely much more matured) than scipp, there is a number of features missing in other situations.
If your use case requires one or several of the items on the following list, using scipp may be worth considering:

- Handling of physical units.
- Propagation of uncertainties.
- Support for histograms, i.e., bin-edge axes, which are by 1 longer than the data extent.
- Support for event data, a particular form of sparse data with 1-D (or N-D) arrays of random-length lists, with very small list entries.
- Support for masks stored with data.
- Written in C++ for better performance (for certain applications), in combination with Python bindings.

Currently scipp is moving from its prototype phase into a more consolidated set of libraries.

Generic functionality of scipp is provided in the core module **scipp**.
Physical units, which are also considered core functionality, are part of the **scipp.units** module.
In addition, more specific functionality is made available in other modules.
Currently the only example for this is **scipp.neutron** for handling data from neutron-scattering experiments.

Documentation
=============

.. toctree::
   :caption: Getting Started
   :maxdepth: 2

   getting-started/overview
   getting-started/faq
   getting-started/installation
   getting-started/quick-start

.. toctree::
   :caption: User Guide
   :maxdepth: 2

   user-guide/data-structures
   user-guide/slicing
   user-guide/computation
   user-guide/masking
   user-guide/sparse-data
   user-guide/groupby
   user-guide/how_to

.. toctree::
   :caption: Visualization
   :maxdepth: 2

   visualization/plotting-overview
   visualization/plotting-in-depth

.. toctree::
   :caption: Tutorials
   :maxdepth: 2

   tutorials/introduction
   tutorials/multi-d-datasets
   tutorials/neutron-data

.. toctree::
   :caption: Reference
   :maxdepth: 2

   python-reference/api
   python-reference/dtype
   python-reference/units
   python-reference/error-propagation

.. toctree::
   :caption: Neutron-scattering
   :maxdepth: 2

   scipp-neutron/overview
   scipp-neutron/groupby
   scipp-neutron/diffraction
   scipp-neutron/from-mantid-to-scipp

.. toctree::
   :caption: C++ Reference
   :maxdepth: 2

   cpp-reference/api
   cpp-reference/customizing
   cpp-reference/transform

.. toctree::
   :caption: About
   :maxdepth: 2

   about/release-notes
