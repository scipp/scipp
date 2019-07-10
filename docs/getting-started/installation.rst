.. _installation:

Installation
============

``pip`` and ``conda`` packages for **scipp** are unfortunately not available yet.

Currently there are two installation options:

1. Installation from source, see the `scipp README <See https://github.com/scipp/scipp/blob/master/README.md>`_.
2. Via the docker container.
   Note that this is an outdated build, before the ongoing major API refactor.

   .. code-block:: sh

      docker pull dmscid/dataset
      docker run -p 8888:8888 dmscid/dataset

   Navigate to ``localhost:8888`` in your browser.
   A number of Jupyter demo notebooks can be found in the ``demo/`` folder.
   These notebooks provide an introduction and basic usage turorial.

We hope to provide proper, working, and up-to-date installation options with an upcoming 0.1 release, within the next couple of months.
