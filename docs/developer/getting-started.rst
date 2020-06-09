Getting Started
===============

Prerequisites
~~~~~~~~~~~~~

See `Tooling <tooling.html>`_ for compilers and other required tools.

Getting the code, building, and installing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Note that this assumes you will end up with a directory structure similar to the following.
If you want something different be sure to modify paths as appropriate.

.. code-block::

  |-- scipp (source code)
  |   |-- build (build directory)
  |   |-- install (Python library installation)
  |   |-- ...
  |-- ...

To build and install the library:

.. code-block:: bash

  # Update Git submodules
  git submodule init
  git submodule update

  # Create build and library install directories
  mkdir build
  mkdir install
  cd build

  # Create Conda environment with dependencies and development tools
  conda env create -f ../scipp-developer.yml            # For Linux
  conda env create -f ../scipp-developer-no-mantid.yml  # For other platforms
  conda activate scipp-developer

To build a debug version of the library:

.. code-block:: bash

  cmake \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPYTHON_EXECUTABLE=$(command -v python3) \
    -DCMAKE_INSTALL_PREFIX=../install \
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=OFF \
    -DDYNAMIC_LIB=ON \
    ..

  # C++ unit tests
  cmake --build . --target all-tests

  # Benchmarks
  cmake --build . --target all-benchmarks

  # Install Python library
  cmake --build . --target install

Alternatively, to build a release version with all optimizations enabled:

.. code-block:: bash

  cmake \
    -GNinja \
    -DPYTHON_EXECUTABLE=$(command -v python3) \
    -DCMAKE_INSTALL_PREFIX=../install \
    -DCMAKE_BUILD_TYPE=Release \
    ..

  cmake --build . --target all-tests
  cmake --build . --target all-benchmarks
  cmake --build . --target install


To use the ``scipp`` Python module:

.. code-block:: bash

  cd ../python
  PYTHONPATH=$PYTHONPATH:./install python3

In Python:

.. code-block:: python

  import scipp as sc

Additional build options
------------------------

1. ``-DDYNAMIC_LIB`` forces the shared libraries building, that also decreases link time.
2. ``-DENABLE_THREAD_LIMIT`` limits the maximum number of threads that TBB can use. This defaults to the maximum number of cores identified on your build system. You may then optionally apply an artificial limit via ``-DTHREAD_LIMIT``.

Running the unit tests
~~~~~~~~~~~~~~~~~~~~~~

Executables for the unit tests can be found in the build directory as ``build/XYZ/test/scipp-XYZ-test``, where ``XYZ`` is the Scipp component under test (e.g. ``core``).
``all-tests`` can be used to build all tests at the same time. Note that simply running ``ctest`` also works, but currently it seems to have an issue with gathering templated tests, so calling the test binaries manually is recommended (and much faster).

To run the Python tests, run (in the ``python/`` directory):

.. code-block:: bash

  cd python
  PYTHONPATH=$PYTHONPATH:./install python3 -m pytest

Building Documentation
----------------------

- Run ``python3 data/fetch_neutron_data.py``
- cd to a directory where the docs should be built (e.g. ``mkdir -p build/docs && cd build/docs``)
- Activate a conda environment with Mantid or ensure Mantid is in your ``PYTHONPATH``
- If Mantid is unavailable (e.g. on Windows) edit ``docs/conf.py`` and include ``nbsphinx_allow_errors = True``. Take care to not commit this change though.
- Run ``sphinx-build scipp/src/docs .``
