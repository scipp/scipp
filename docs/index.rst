|data-structures| |plotting| |masking|
|binning| |slicing| |scipp-neutron|

.. |data-structures| image:: _static/title-repr-html.png
   :width: 33%
   :target: user-guide/data-structures.html

.. |binning| image:: _static/title-binning.png
   :width: 33%
   :target: user-guide/binned-data/binned-data.html

.. |masking| image:: _static/title-masking.png
   :width: 33%
   :target: user-guide/masking.html

.. |plotting| image:: _static/title-plotting.png
   :width: 33%
   :target: visualization/plotting-overview.html

.. |scipp-neutron| image:: _static/title-instrument-view.png
   :width: 33%
   :target: https://scipp.github.io/scippneutron

.. |slicing| image:: _static/title-show.png
   :width: 33%
   :target: user-guide/slicing.html

scipp - Multi-dimensional data arrays with labeled dimensions
=============================================================

A Python library enabling a modern and intuitive way of working with scientific data in Jupyter notebooks
---------------------------------------------------------------------------------------------------------

**scipp** is heavily inspired by `xarray <https://xarray.pydata.org>`_.
It enriches raw NumPy-like multi-dimensional arrays of data by adding named dimensions and associated coordinates.
Multiple arrays can be combined into datasets.
While for many applications xarray is certainly more suitable (and definitely much more matured) than scipp, there is a number of features missing in other situations.
If your use case requires one or several of the items on the following list, using scipp may be worth considering:

- **Physical units** are stored with each data or coord array and are handled in arithmetic operations.
- **Propagation of uncertainties**.
- Support for **histograms**, i.e., **bin-edge axes**, which are by 1 longer than the data extent.
- Support for scattered data and **non-destructive binning**.
  This includes first and foremost **event data**, a particular form of sparse data with arrays of random-length lists, with very small list entries.
- Support for **masks stored with data**.
- Internals written in C++ for better performance (for certain applications), in combination with Python bindings.

Generic functionality of scipp is provided in the **scipp** Python package.
In addition, more specific functionality is made available in other packages.
Currently the only example for this is `scippneutron <https://scipp.github.io/scippneutron>`_ for handling data from neutron-scattering experiments.

News
----

- [|SCIPP_RELEASE_MONTH|] scipp-|SCIPP_VERSION| `has been released <about/release-notes.rst>`_.
  Check out the `What's new <about/whats-new.rst>`_ notebook for an overview of recent highlights and major changes.
- Scipp changed from GPLv3 to the more permissive BSD-3 license which fits better into the Python eco system.
- Looking for ``scipp.neutron``?
  This submodule has been moved into its own package, `scippneutron <https://scipp.github.io/scippneutron>`_.
- [June 2021] `scipp-0.7 <https://scipp.github.io/about/release-notes.html#v0-7-0-june-2021>`_ has been released.
  The `What's new 0.7 <https://scipp.github.io/release/0.7.0/about/whats-new/whats-new-0.7.0.html>`_ notebook provides an overview of the highlights and major changes.
- [March 2021] `scipp-0.6 <https://scipp.github.io/about/release-notes.html#v0-6-0-march-2021>`_ has been released.
  The `What's new 0.6 <https://scipp.github.io/release/0.6.0/about/whats-new/whats-new-0.6.0.html>`_ notebook provides an overview of the highlights and major changes.
- [Janunary 2021] `scipp-0.5 <https://scipp.github.io/about/release-notes.html#v0-5-0-january-2021>`_ has been released.
  The `What's new 0.5 <https://scipp.github.io/release/0.5.0/about/whats-new/whats-new-0.5.0.html>`_ notebook provides an overview of the highlights and major changes.

Where can I get help?
---------------------

For questions not answered in the documentation
`this page <https://github.com/scipp/scipp/issues?utf8=%E2%9C%93&q=label%3Aquestion>`_
provides a forum with discussions on problems already met/solved in the community.

New questions can be asked by
`opening <https://github.com/scipp/scipp/issues/new?assignees=&labels=question&template=question.md&title=>`_
a new |QuestionLabel|_ issue.

.. |QuestionLabel| image:: images/question.png
.. _QuestionLabel: https://github.com/scipp/scipp/issues/new?assignees=&labels=question&template=question.md&title=

Documentation
=============

.. toctree::
   :caption: Getting Started
   :maxdepth: 3

   getting-started/overview
   getting-started/installation
   getting-started/quick-start
   getting-started/faq

.. toctree::
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
   :caption: Visualization
   :maxdepth: 3

   visualization/representations-and-tables
   visualization/plotting-overview
   visualization/plotting-in-depth

.. toctree::
   :caption: Reference
   :maxdepth: 3

   reference/api
   reference/linear-algebra
   reference/dtype
   reference/units
   reference/error-propagation
   reference/runtime-configuration
   reference/developer-documentation

.. toctree::
   :caption: Tutorials
   :maxdepth: 3

   tutorials/introduction
   tutorials/multi-d-datasets

.. toctree::
   :caption: About
   :maxdepth: 3

   about/about
   about/whats-new
   about/roadmap
   about/contributing
   about/release-notes
