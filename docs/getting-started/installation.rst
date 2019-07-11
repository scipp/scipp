.. _installation:

Installation
============

Conda
-----

To create a new ``conda`` environment with ``scipp``:

.. code-block:: sh

   conda create -n env_with_scipp -c scipp scipp

Note that these packages from `Anaconda Cloud <https://conda.anaconda.org/scipp>`_ are currently only available on Linux.

Docker
------

A docker container is available.
Note that this is an outdated build, before the ongoing major API refactor.

.. code-block:: sh

   docker pull dmscid/dataset
   docker run -p 8888:8888 dmscid/dataset

Navigate to ``localhost:8888`` in your browser.
A number of Jupyter demo notebooks can be found in the ``demo/`` folder.
These notebooks provide an introduction and basic usage turorial.

From source
-----------

See the `scipp README <See https://github.com/scipp/scipp/blob/master/README.md>`_.
