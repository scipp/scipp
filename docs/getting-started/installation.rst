.. _installation:

Installation
============

Scipp requires Python 3.7 or above.

Conda
-----

The easiest way to install ``scipp`` is using `conda <https://conda.io>`_.
Packages from `Anaconda Cloud <https://conda.anaconda.org/scipp>`_ are available for Linux, macOS, and Windows.
It is recommended to create an environment rather than installing individual packages.

With the provided environment file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Download :download:`scipp.yml <../environments/scipp.yml>` for the stable release version of scipp.
   Use :download:`scipp-latest.yml <../environments/scipp-latest.yml>` for the latest development version.
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

If you have previously installed scipp with conda we nevertheless recommend creating a fresh environment rather than trying to ``conda update``.
You may want to remove your old environment first, e.g.,

.. code-block:: sh

   conda activate
   conda env remove -n scipp

and then proceed as per instructions above.
The ``conda activate`` ensures that you are in your ``base`` environment.

Without the provided environment file
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To create a new conda environment with scipp:

.. code-block:: sh

   conda create -n env_with_scipp -c conda-forge -c scipp scipp

To add scipp to an existing conda environment:

.. code-block:: sh

   conda install -c conda-forge -c scipp scipp

For a more up-to-date version, the `scipp/label/dev` channel can be used instead:

.. code-block:: sh

   conda install -c conda-forge -c scipp/label/dev scipp

.. note::
   Installing ``scipp`` on Windows requires ``Microsoft Visual Studio 2019 C++ Runtime`` installed.
   Visit https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads for the up to date version of the library.

After installation the module ``scipp`` can be imported in Python.
Note that only the bare essential dependencies are installed.
If you wish to use plotting functionality you will also need to install ``matplotlib`` and ``ipywidgets``.

To update or remove ``scipp`` use `conda update <https://docs.conda.io/projects/conda/en/latest/commands/update.html>`_ and `conda remove <https://docs.conda.io/projects/conda/en/latest/commands/remove.html>`_.

From source
-----------

See `developer getting started <../developer/getting-started.html>`_.
