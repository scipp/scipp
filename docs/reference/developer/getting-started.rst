Getting Started
===============

Prerequisites
~~~~~~~~~~~~~

All non-optional build dependencies are installed automatically through Conan when running CMake.
Conan itself can be installed manually but we recommend using the provided ``scipp-developer.yml``
for installing this and other dependencies in a ``conda`` environment (see below).
Alternatively you can refer to this file for a full list of dependencies.

See `Tooling <tooling.rst>`_ for compilers and other required tools.

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
  conda activate scipp-developer

To build a debug version of the library:

.. code-block:: bash

  cmake \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPython_EXECUTABLE=$(command -v python3) \
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
    -DPython_EXECUTABLE=$(command -v python3) \
    -DCMAKE_INSTALL_PREFIX=../install \
    -DCMAKE_BUILD_TYPE=Release \
    ..

  cmake --build . --target all-tests
  cmake --build . --target all-benchmarks
  cmake --build . --target install


To use the ``scipp`` Python module:

.. code-block:: bash

  cd ../python
  PYTHONPATH=$PYTHONPATH:../install python3

In Python:

.. code-block:: python

  import scipp as sc

Additional build options
------------------------

1. ``-DDYNAMIC_LIB`` forces the shared libraries building, that also decreases link time.
2. ``-DENABLE_THREAD_LIMIT`` limits the maximum number of threads that TBB can use. This defaults to the maximum number of cores identified on your build system. You may then optionally apply an artificial limit via ``-DTHREAD_LIMIT``.
3. ``-DTHREADING`` enable or disable multi-threading. ``ON`` by default.
4. ``-DPRECOMPILED_HEADERS`` toggle usage of precompiled headers. ``OFF`` by default.
5. ``-DCPPCHECK`` toggle run of cppcheck during compilation. ``OFF`` by default.
6. ``-DCTEST_DISCOVER_TESTS`` toggle discovery of individual tests for better (but much slower) integration with ``ctest``. ``OFF`` by default.

Running the unit tests
~~~~~~~~~~~~~~~~~~~~~~

Executables for the unit tests can be found in the build directory as ``build/bin/scipp-XYZ-test``, where ``XYZ`` is the Scipp component under test (e.g. ``core``).
``all-tests`` can be used to build all tests at the same time. Note that simply running ``ctest`` also works, but currently it seems to have an issue with gathering templated tests, so calling the test binaries manually is recommended (and much faster).

To run the Python tests, run (in the ``python/`` directory):

.. code-block:: bash

  cd python
  PYTHONPATH=$PYTHONPATH:./install python3 -m pytest

Building Documentation
~~~~~~~~~~~~~~~~~~~~~~

- Run ``cmake --build . --target docs`` from your build directory.
- This will build the documentation and put it on ``<build dir>/docs``.
- If rebuilding the documentation is slow it can be quicker to remove the docs build directory and start a fresh build.

Precommit Hooks
~~~~~~~~~~~~~~~

If you wish, you can install precommit hooks for flake8 and yapf. In the source directory run:

.. code-block:: bash

  pre-commit install
  pre-commit run --all-files

Using scipp as a C++ library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Using Scipp as a C++ library is not recommened at this point as the API (and ABI) is not stable and documentation is sparse.
Nonetheless, it can be used as a ``cmake`` package as follows.
In your ``CMakeLists.txt``:

.. code-block:: cmake

  find_package(Scipp 0.5 REQUIRED) # replace with required version
  target_link_libraries(mytarget PUBLIC scipp::dataset)

If scipp was install using ``conda``, ``cmake`` should find it automatically.
If you build and installed scipp from source use, e.g.,:

.. code-block:: bash

  cmake -DCMAKE_PREFIX_PATH=<your_scipp_install_dir>

where ``<your_scipp_install_dir>`` should point to the ``CMAKE_INSTALL_PREFIX`` that was used when building ``scipp``.
Alternative set the ``Scipp_DIR`` or ``CMAKE_PREFIX_PATH`` (environment) variables to this path.

Generating coverage reports
~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Run ``cmake`` with options ``-DCOVERAGE=On -DCMAKE_BUILD_TYPE=Debug``.
- Run ``cmake --build . --target coverage`` from your build directory.
- Open ``coverage/index.html`` in a browser.
