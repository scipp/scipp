Getting Started
===============

Prerequisites
~~~~~~~~~~~~~

See `Tooling <tooling.html>`_ for compilers and other required tools.

Scipp uses TBB for multi-threading.
This is an optional dependency.
We have found that TBB from ``conda-forge`` works best in terms of CMake integration.
You need ``tbb`` and ``tbb-devel``.

Getting the code, building, and installing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Note that Python dependencies are managed via Conda during the packaging step.
In order to build the Python exports manually it is recommended to use a Conda environment for building and install dependencies manually from the list in `meta.yml <https://github.com/scipp/scipp/blob/master/conda/meta.yaml>`_.

To build and install the library:

.. code-block:: bash

  git submodule init
  git submodule update

  mkdir -p build/install
  cd build

  conda create -n scipp python=3.7
  conda env activate scipp
  conda install # populate list here

  cmake \
    -GNinja \
    -DPYTHON_EXECUTABLE=$(command -v python3) \
    -DCMAKE_INSTALL_PREFIX=../install \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF \
    -DDYNAMIC_LIB=ON \
    ..

  cmake --build . --target all-tests
  cmake --build . --target install

To use the ``scipp`` Python module:

.. code-block:: bash

  cd ../python
  export PYTHONPATH=$PYTHONPATH:../install

In Python:

.. code-block:: python

  import scipp as sc

Additional build options
------------------------

1. ``-DDYNAMIC_LIB`` forces the shared libraries building, that also decreases link time.
2. ``-DENABLE_THREAD_LIMIT`` limits the maximum number of threads that TBB can use. This defaults to the maximum number of cores identified on your build system. You may then optionally apply an artificial limit via ``-DTHREAD_LIMIT``. 

Running the unit tests
~~~~~~~~~~~~~~~~~~~~~~

To run the C++ tests, run (in the ``build/`` directory):

.. code-block:: bash

  ./units/test/scipp-units-test
  ./core/test/scipp-core-test

Note that simply running ``ctest`` also works, but currently it seems to have an issue with gathering templated tests, so calling the test binaries manually is recommended (and much faster).

To run the Python tests, run (in the ``python/`` directory):

.. code-block:: bash

  cd python
  python3 -m pip install -r requirements.txt
  python3 -m pytest
