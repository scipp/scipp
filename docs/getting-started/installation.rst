.. _installation:

Installation
============

There are three installation options:

1. Installation from source, see the `scipp README <See https://github.com/scipp/scipp/blob/master/README.md>`_.
2. Installation via ``conda`` using packages from `Anaconda Cloud <https://conda.anaconda.org/scipp>`_.
   To create a new environemtn for trying scipp out: ``conda create -n env_with_scipp -c scipp scipp``.
3. Via the docker container.
   Note that this is an outdated build, before the ongoing major API refactor.

   .. code-block:: sh

      docker pull dmscid/dataset
      docker run -p 8888:8888 dmscid/dataset

   Navigate to ``localhost:8888`` in your browser.
   A number of Jupyter demo notebooks can be found in the ``demo/`` folder.
   These notebooks provide an introduction and basic usage turorial.
