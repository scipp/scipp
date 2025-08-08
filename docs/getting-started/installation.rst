.. _installation:

Installation
============

Scipp requires Python 3.11 or above.

Conda
-----

The easiest way to install Scipp is using `conda <https://docs.conda.io>`_.
Packages from `conda-forge <https://anaconda.org/conda-forge/scipp>`_ (or from `Anaconda Cloud <https://anaconda.org/scipp/scipp>`_) are available for Linux, macOS, and Windows.
It is recommended to create an environment rather than installing individual packages.

With the provided environment file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Download :download:`scipp.yml <../environments/scipp.yml>` for the latest stable release version of Scipp.
2. In a terminal run:

   .. code-block:: sh

      conda activate
      conda env create -f scipp.yml
      conda activate scipp
      jupyter lab

   The ``conda activate`` ensures that you are in your ``base`` environment.
   This will take a few minutes.
   Above, replace ``scipp.yml`` with the path to the download location you used to download the environment.
   Open the link printed by Jupyter in a browser if it does not open automatically.

If you are new to Scipp, continue reading with `Quick Start <quick-start.rst>`_ and `Data Structures <../user-guide/data-structures.rst>`_.

If you have previously installed Scipp with conda we nevertheless recommend creating a fresh environment rather than trying to ``conda update``.
You may want to remove your old environment first, e.g.,

.. code-block:: sh

   conda activate
   conda env remove -n scipp

and then proceed as per instructions above.
The ``conda activate`` ensures that you are in your ``base`` environment.

Without the provided environment file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To create a new conda environment with Scipp:

.. code-block:: sh

   conda create -n env_with_scipp -c conda-forge -c scipp scipp

To add Scipp to an existing conda environment:

.. code-block:: sh

   conda install -c conda-forge -c scipp scipp

.. note::
   Installing Scipp on Windows requires ``Microsoft Visual Studio 2019 C++ Runtime`` (and versions above) installed.
   Visit https://support.microsoft.com/en-us/topic/the-latest-supported-visual-c-downloads-2647da03-1eea-4433-9aff-95f26a218cc0 for the up to date version of the library.

After installation the module, Scipp can be imported in Python.
Note that only the bare essential dependencies are installed.
If you wish to use plotting functionality you will also need to install ``plopp``, ``ipympl``, and ``pythreejs``.

To update or remove Scipp use `conda update <https://docs.conda.io/projects/conda/en/latest/commands/update.html>`_ and `conda remove <https://docs.conda.io/projects/conda/en/latest/commands/remove.html>`_.

Pip
---

Scipp is available from `PyPI <https://pypi.org/>`_ via ``pip``:

.. code-block:: sh

   pip install scipp

By default, this is only a minimal install without optional dependencies.
To install all optional dependencies, including libraries for interactive plotting in Jupyter, use:

.. code-block:: sh

   pip install scipp[all]


You can also leave out the interactive tools and bring in only functional optional dependencies,
such as ``h5py`` and ``scipy``, use ``extra`` instead of ``all``:

.. code-block:: sh

   pip install scipp[extra]

From source
-----------

See the `Getting Started <../development/getting-started.rst>`_ page in the Development section.
