.. _installation:

Installation
============

*Note: Currently all pre-built packages of scipp come with the "neutron" set of units and dimension labels.
Once multiple options are available (or required) we will look into either shipping multiple binaries, or allow for configuration at run time.
If you want to use scipp and require different units/dimensions, please see* :ref:`customizing`, *or better, get in touch with us.*

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

If you wish to use Scipp with Mantid you may use the following command to create an environment containing both Scipp and Mantid framework.

Note that Conda packages for Mantid are only available on Linux and are currently maintained seperate to the Mantid project.
This is due to some dependencies being too old to work in the same environment as Scipp.

.. code-block:: sh

  $ conda create \
      -n env_with_scipp_and_mantid \
      -c conda-forge \
      -c dannixon \
      -c scipp/label/dev \
      python=3.7 \
      scipp \
      mantid-framework

.. note::
   Instaling Scipp with Mantid on Windows is possible but requires ``Windows Subsystem for Linux 1`` (WSL 1) installed and is limited to Windows 10.
   Please follow the steps on the `Windows Subsystem for Linux Installation Guide page <https://docs.microsoft.com/en-us/windows/wsl/install-win10>`_
   to enable Linux support.
   Once ``WSL 1`` is installed, setting up Scipp with Mantid follows the Linux specific directions described above.

From source
-----------

See `developer getting started <../developer/getting-started.html>`_.
