.. _installation:

Installation
============

Conda
-----

The easiest way to install ``scipp`` is using `conda <https://conda.io>`_.
To create a new conda environment with scipp:

.. code-block:: sh

   $ conda create -n env_with_scipp -c scipp/label/dev scipp

To add scipp to an existing conda environment:

.. code-block:: sh

   $ conda install -c scipp/label/dev scipp

We are currently recommending the `scipp/label/dev` channel instead of just `scipp` since the latter is not useful until we have had our first proper release.
Note that these packages from `Anaconda Cloud <https://conda.anaconda.org/scipp>`_ are currently only available on Linux.

After installation the module ``scipp`` can be imported in Python.

To update or remove ``scipp`` use `conda update <https://docs.conda.io/projects/conda/en/latest/commands/update.html>`_ and `conda remove <https://docs.conda.io/projects/conda/en/latest/commands/remove.html>`_.

Docker
------

A docker container is available.
Note that this is an outdated build, before the ongoing major API refactor:

.. code-block:: sh

   docker pull dmscid/dataset
   docker run --rm -p 127.0.0.1:8888:8888 dmscid/dataset

Navigate to ``localhost:8888`` in your browser.
A number of Jupyter demo notebooks can be found in the ``demo/`` folder.
These notebooks provide an introduction and basic usage turorial.

Getting Started with Docker
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Follow the instructions `here <https://docs.docker.com/install/>`_ to install Docker CE on your system.

Linux
#####

If this is the first time using Docker there are additional setup steps:

  1. Add your use to the ``docker`` group (this avoids having to use ``sudo`` with Docker commands): ``sudo usermod -aG docker $(whoami)``
  2. Ensure that the Docker daemon is set to automatically start and is currently running: ``sudo systemctl enable docker && sudo systemctl start docker``

Start a container using the command: ``docker run --rm -p 127.0.0.1:8888:8888 dmscid/dataset``.

Access the container in a browser using the address: ``http://127.0.0.1:8888``.

Windows & MacOS
###############

Start a container using the command: ``docker run --rm -p $(docker-machine ip $(docker-machine active)):8888:8888 dmscid/dataset``.

Access the container in a browser using the address: ``http://<ip>:8888``, where ``<ip>`` is determined using the command ``docker-machine ip $(docker-machine active)``.

From source
-----------

See the `scipp README <https://github.com/scipp/scipp/blob/master/README.md>`_.
