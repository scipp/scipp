scipp - Multi-dimensional data arrays with labeled dimensions
=============================================================

.. raw:: html

   <span style="font-size:1.2em;font-style:italic;color:#5a5a5a">
      A Python library enabling a modern and intuitive way of working with scientific data in Jupyter notebooks
   </span>

|data-structures| |plotting| |masking|
|binning| |slicing| |scipp-neutron|

.. |data-structures| image:: _static/title-repr-html.png
   :width: 32%
   :target: user-guide/data-structures.html

.. |binning| image:: _static/title-binning.png
   :width: 32%
   :target: user-guide/binned-data/binned-data.html

.. |masking| image:: _static/title-masking.png
   :width: 32%
   :target: user-guide/masking.html

.. |plotting| image:: _static/title-plotting.png
   :width: 32%
   :target: visualization/plotting-overview.html

.. |scipp-neutron| image:: _static/title-instrument-view.png
   :width: 32%
   :target: https://scipp.github.io/scippneutron

.. |slicing| image:: _static/title-show.png
   :width: 32%
   :target: user-guide/slicing.html

**scipp** is heavily inspired by `xarray <https://xarray.pydata.org>`_.
It enriches raw NumPy-like multi-dimensional arrays of data by adding named dimensions and associated coordinates.
Multiple arrays can be combined into datasets.
While for many applications xarray is certainly more suitable (and definitely much more matured) than Scipp, there is a number of features missing in other situations.
If your use case requires one or several of the items on the following list, using Scipp may be worth considering:

- **Physical units** are stored with each data or coord array and are handled in arithmetic operations.
- **Propagation of uncertainties**.
- Support for **histograms**, i.e., **bin-edge axes**, which are by 1 longer than the data extent.
- Support for scattered data and **non-destructive binning**.
  This includes first and foremost **event data**, a particular form of sparse data with arrays of random-length lists, with very small list entries.
- Support for **masks stored with data**.
- Internals written in C++ for better performance (for certain applications), in combination with Python bindings.

Generic functionality of Scipp is provided in the **scipp** Python package.
In addition, more specific functionality is made available in other packages.
Examples for this are `scippnexus <https://scipp.github.io/scippnexus>`_ for loading NeXus files, `scippneutron <https://scipp.github.io/scippneutron>`_ for handling data from neutron-scattering experiments,
and `ess <https://scipp.github.io/ess>`_ for dealing with the specifics of neutron instruments at ESS.

News
----

- [|SCIPP_RELEASE_MONTH|] scipp-|SCIPP_VERSION| `has been released <about/release-notes.rst>`_.
  Check out the `What's new <about/whats-new.rst>`_ notebook for an overview of recent highlights and major changes.
- Is your data stored in NeXus files?
  Checkout our new project `scippnexus <https://scipp.github.io/scippnexus>`_, a h5py-like utility for conveniently loading NeXus classes into Scipp data structures.
- We now provide ``pip`` packages of ``scipp``, in addition to ``conda`` packages.
  See `Installation <getting-started/installation.rst#Pip>`_ for details.
- Scipp changed from GPLv3 to the more permissive BSD-3 license which fits better into the Python eco system.
- Looking for ``scipp.neutron``?
  This submodule has been moved into its own package, `scippneutron <https://scipp.github.io/scippneutron>`_.

Lost? New to Scipp? Start Here!
-------------------------------

The **Getting Started** section motivates Scipp in `What is Scipp? <getting-started/overview.rst>`_, provides installation instructions in `Installation <getting-started/installation>`_, and gives a brief overview in `Quick start <getting-started/quick-start>`_.

The **User Guide** provides a high-level overview of the most important Scipp concepts and features.
Read `Data Structures <user-guide/data-structures.html>`_, `Slicing <user-guide/slicing.html>`_, and `Computation <user-guide/computation.html>`_ (in this order) to develop an understanding of the core concepts of Scipp.

Further sections of the User Guide are optional and can be studied in arbitrary order.
Depending on your area of application you may be most interested in `Binned Data <user-guide/binned-data.rst>`_, `Coordinate Transformations <user-guide/coordinate-transformations.rst>`_,
`GroupBy <user-guide/groupby.rst>`_, or
`Masking <user-guide/masking.rst>`_.
The combination of these features is what sets Scipp apart from other Python libraries.
Make sure to also checkout `Representations and Tables <visualization/representations-and-tables.rst>`_ and `Plotting Overview <visualization/plotting-overview.rst>`_ for an overview of Scipp's powerful visualization features.

The **Reference** documentation section provides a detailed listing of all functions and classes of Scipp, as well as some more technical documentation.
If you are looking for something in particular, use the **Search the docs** function in the left navigation panel.

Where can I get help?
---------------------

We strive to keep our documentation complete and up-to-date.
However, we cannot cover all use-cases and questions users may have.

We use GitHub's `discussions <https://github.com/scipp/scipp/discussions>`_ forum for questions
that are not answered by these documentation pages.
This space can be used to both search through problems already met/solved in the community
and open new discussions if none of the existing ones provide a satisfactory answer.

.. toctree::
   :hidden:
   :caption: Getting Started
   :maxdepth: 3

   getting-started/overview
   getting-started/installation
   getting-started/quick-start
   getting-started/faq

.. toctree::
   :hidden:
   :caption: User Guide
   :maxdepth: 3

   user-guide/data-structures
   user-guide/slicing
   user-guide/computation
   user-guide/masking
   user-guide/binned-data
   user-guide/groupby
   user-guide/coordinate-transformations
   user-guide/reading-and-writing-files
   user-guide/how_to
   user-guide/tips-tricks-and-anti-patterns

.. toctree::
   :hidden:
   :caption: Visualization
   :maxdepth: 3

   visualization/representations-and-tables
   visualization/plotting-overview
   visualization/plotting-in-depth

.. toctree::
   :hidden:
   :caption: Reference
   :maxdepth: 3

   reference/classes
   reference/creation-functions
   reference/free-functions
   reference/modules
   reference/linear-algebra
   reference/dtype
   reference/units
   reference/error-propagation
   reference/ownership-mechanism-and-readonly-flags
   reference/logging
   reference/runtime-configuration
   reference/developer-documentation

.. toctree::
   :hidden:
   :caption: Tutorials
   :maxdepth: 3

   tutorials/from-tabular-data-to-binned-data
   tutorials/solar_flares

.. toctree::
   :hidden:
   :caption: About
   :maxdepth: 3

   about/about
   about/whats-new
   about/roadmap
   about/contributing
   about/release-notes
