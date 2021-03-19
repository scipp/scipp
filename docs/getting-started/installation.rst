.. _installation:

Installation
============

Scipp requires Python 3.7 or above.

Conda
-----

The easiest way to install ``scipp`` is using `conda <https://conda.io>`_.
Packages from `Anaconda Cloud <https://conda.anaconda.org/scipp>`_ are available for Linux, macOS and Windows.

To create a new conda environment with scipp:

.. code-block:: sh

   $ conda create -n env_with_scipp -c conda-forge -c scipp scipp

To add scipp to an existing conda environment:

.. code-block:: sh

   $ conda install -c conda-forge -c scipp scipp

For a more up-to-date version, the `scipp/label/dev` channel can be used instead:

.. code-block:: sh

   $ conda install -c conda-forge -c scipp/label/dev scipp

.. note::
   Instaling ``scipp`` on Windows requires ``Microsoft Visual Studio 2019 C++ Runtime`` installed.
   Visit https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads for the up to date version of the library.

After installation the module ``scipp`` can be imported in Python.
Note that only the bare essential dependencies are installed.
If you wish to use plotting functionality you will also need to install ``matplotlib`` and ``ipywidgets``.

To update or remove ``scipp`` use `conda update <https://docs.conda.io/projects/conda/en/latest/commands/update.html>`_ and `conda remove <https://docs.conda.io/projects/conda/en/latest/commands/remove.html>`_.

From source
-----------

See `developer getting started <../developer/getting-started.html>`_.
