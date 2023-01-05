Getting Started
===============

Prerequisites
~~~~~~~~~~~~~

All non-optional build dependencies are installed automatically through Conan when running CMake.
Conan itself can be installed manually but we recommend using the ``developer.yml`` file
for installing this and other dependencies in a ``conda`` environment (see below).
Alternatively you can refer to this file for a full list of dependencies.

See `Tooling <tooling.rst>`_ for compilers and other required tools.

Getting the code, building, and installing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You first need to clone the git repository (either via SSH or HTTPS) from `GitHub <https://github.com/scipp/scipp>`_.
Note that this assumes you will end up with a directory structure similar to the following.
If you want something different be sure to modify paths as appropriate.

.. code-block::

  |-- /home/user/scipp (source code)
  |   |-- build (build directory)
  |   |-- install (Python library installation)
  |   |-- ...
  |-- ...

To build and install the library:

.. code-block:: bash

  # Create Conda environment with dependencies and development tools
  conda env create -f docs/environments/developer.yml
  conda activate scipp-developer

  # Update Git submodules
  git submodule init
  git submodule update

  # Create build and library install directories
  mkdir build
  mkdir install
  cd build

If you are running on Windows, you need to use a visual studio developer command prompt for the following steps. This can be opened manually from the start menu, or programmatically by calling the appropriate vcvars script, for example:

.. code-block:: bash

  "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"

If you wish to build using the Visual Studio CMake generators instead, there is a ``windows-msbuild`` CMake preset for this purpose.

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

  conda develop /home/user/scipp/install

In Python:

.. code-block:: python

  import scipp as sc

Instead of ``conda develop``, some developers may prefer using ``pip`` to make an editable install.
Scipp uses ``scikit-build``, which currently does not fully support this directly.
Therefore, we need to call ``cmake`` manually in this case and install into the Python source directory:

.. code-block:: bash

  cmake --preset base -DCONAN_TBB=ON -DCMAKE_INSTALL_PREFIX=src/
  cmake --build --preset build
  pip install -e .

Above we used some of the ``cmake`` presets, but you may also call ``cmake`` without those for more control of the options.
The crucial part is that the install must be performed into ``src/``.
You can now use the editable install as usual, i.e., changes to Python files of Scipp are directly visible when importing Scipp, without the need for a new install.
When making changes to the C++ side of Scipp, you will need to re-run the ``install`` target using ``cmake``, e.g.,

.. code-block:: bash

  cmake --build --preset build

Additional build options
------------------------

1. ``-DDYNAMIC_LIB`` forces the shared libraries building, that also decreases link time.
2. ``-DTHREADING`` enable or disable multi-threading. ``ON`` by default.
3. ``-DPRECOMPILED_HEADERS`` toggle usage of precompiled headers. ``OFF`` by default.
4. ``-DCPPCHECK`` toggle run of cppcheck during compilation. ``OFF`` by default.
5. ``-DCTEST_DISCOVER_TESTS`` toggle discovery of individual tests for better (but much slower) integration with ``ctest``. ``OFF`` by default.

Running the unit tests
~~~~~~~~~~~~~~~~~~~~~~

Executables for the unit tests can be found in the build directory as ``build/bin/scipp-XYZ-test``, where ``XYZ`` is the Scipp component under test (e.g. ``core``).
``all-tests`` can be used to build all tests at the same time. Note that simply running ``ctest`` also works, but currently it seems to have an issue with gathering templated tests, so calling the test binaries manually is recommended (and much faster).

To run the Python tests, run (in the top-level ``/home/user/scipp`` directory):

.. code-block:: bash

  conda develop /home/user/scipp/install
  python -m pytest tests


Building Documentation
~~~~~~~~~~~~~~~~~~~~~~

Run

.. code-block:: bash

  tox -e lib
  tox -e docs


This will build the HTML documentation and put it in a folder named ``html``.
If rebuilding the documentation is slow, it can be quicker to remove the docs build directory and start a fresh build.

Precommit Hooks
~~~~~~~~~~~~~~~

If you wish, you can install precommit hooks for flake8 and yapf. In the source directory run:

.. code-block:: bash

  pre-commit install
  pre-commit run --all-files

Using Scipp as a C++ library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Using Scipp as a C++ library is not recommended at this point as the API (and ABI) is not stable and documentation is sparse.
Nonetheless, it can be used as a ``cmake`` package as follows.
In your ``CMakeLists.txt``:

.. code-block:: cmake

  find_package(Scipp 0.11 REQUIRED) # replace with required version
  target_link_libraries(mytarget PUBLIC scipp::dataset)

If Scipp was install using ``conda``, ``cmake`` should find it automatically.
If you build and installed Scipp from source use, e.g.,:

.. code-block:: bash

  cmake -DCMAKE_PREFIX_PATH=<your_scipp_install_dir>

where ``<your_scipp_install_dir>`` should point to the ``CMAKE_INSTALL_PREFIX`` that was used when building Scipp.
Alternative set the ``Scipp_DIR`` or ``CMAKE_PREFIX_PATH`` (environment) variables to this path.

Generating coverage reports
~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Run ``cmake`` with options ``-DCOVERAGE=On -DCMAKE_BUILD_TYPE=Debug``.
- Run ``cmake --build . --target coverage`` from your build directory.
- Open ``coverage/index.html`` in a browser.
