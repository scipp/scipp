Getting Started
===============

TL;DR
-----

Standard setup
~~~~~~~~~~~~~~

.. code-block:: bash

  # Get the code
  git clone git@github.com:scipp/scipp.git && cd scipp
  git submodule init
  git submodule update

  # Setup dev env (installs all dependencies via pixi)
  pixi install
  pre-commit install
  pre-commit run --all-files

  # Build, install, and run C++ tests
  pixi run cpp-test

  # Setup editable install and run Python tests
  pixi run editable
  python -m pytest tests

You should now also be able to run ``pixi run python -c 'import scipp as sc'``.

Common tasks
~~~~~~~~~~~~

All of the below assume that the standard setup outlined above was performed.

Build the docs:

.. code-block:: bash

   pixi run docs

   # To clean and build all docs

   pixi run docs-clean

Static analysis and formatting:

.. code-block:: bash

   pixi run static

Type checking:

.. code-block:: bash

   pixi run mypy

Overview
--------

We use `pixi <https://pixi.sh>`_ to manage the developer environment.
All dependencies (C++ build tools, Python packages, testing, docs) are defined in ``pixi.toml`` at the repository root.
See `Tooling <tooling.rst>`_ for compilers and other required tools.

Getting the code
----------------

Clone the git repository (either via SSH or HTTPS) from `GitHub <https://github.com/scipp/scipp>`_.

.. code-block:: bash

  git clone git@github.com:scipp/scipp.git
  cd scipp

  # Update Git submodules
  git submodule init
  git submodule update

  # Install the pixi environment
  pixi install

Pre-commit Hooks
----------------

We use ``pre-commit`` for static analysis and code formatting.
Install the pre-commit hooks to avoid committing non-compliant code.
In the source directory run:

.. code-block:: bash

  pre-commit install
  pre-commit run --all-files

Building Scipp
--------------

As big parts of Scipp are written in C++, we use CMake for building.
This assumes you will end up with a directory structure similar to the following.
If you want something different be sure to modify paths as appropriate:

.. code-block::

  |-- /home/user/scipp (source code)
  |   |-- build (build directory)
  |   |-- install (Python library installation)
  |   |-- ...
  |-- ...

To build and install the library using pixi tasks:

.. code-block:: bash

  # Configure, build, and run C++ tests in one step
  pixi run cpp-test

  # Or run individual steps
  pixi run configure       # cmake --preset base
  pixi run build           # cmake --build --preset build (depends on configure)

To build a debug version:

.. code-block:: bash

  pixi run configure-debug
  pixi run build-debug

If you are running on Windows, you need to use a visual studio developer command prompt for the following steps. This can be opened manually from the start menu, or programmatically by calling the appropriate vcvars script, for example:

.. code-block:: bash

  "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"

If you wish to build using the Visual Studio CMake generators instead, there is a ``windows-msbuild`` CMake preset for this purpose.

Some developers may prefer an "editable" install, i.e., with changes to Python files in the ``src`` directly becoming visible without reinstalling.
The ``editable`` pixi task creates symlinks from the install directory into the source tree:

.. code-block:: bash

  pixi run editable  # builds first, then creates symlinks

You can now use the editable install as usual, i.e., changes to Python files of Scipp are directly visible when importing Scipp, without the need for a new install.
When making changes to the C++ side of Scipp, you will need to re-run the ``install`` target using ``cmake``, e.g.,

.. code-block:: bash

  pixi run build

You can also run cmake commands directly within the pixi environment:

.. code-block:: bash

  pixi run -- cmake --preset base
  pixi run -- cmake --build --preset build


Additional build options
~~~~~~~~~~~~~~~~~~~~~~~~

1. ``-DDYNAMIC_LIB`` forces the shared libraries building, that also decreases link time.
2. ``-DTHREADING`` enable or disable multi-threading. ``ON`` by default.
3. ``-DPRECOMPILED_HEADERS`` toggle usage of precompiled headers. ``OFF`` by default.
4. ``-DCPPCHECK`` toggle run of cppcheck during compilation. ``OFF`` by default.
5. ``-DCTEST_DISCOVER_TESTS`` toggle discovery of individual tests for better (but much slower) integration with ``ctest``. ``OFF`` by default.
6. ``-DSKIP_REMOTE_SOURCES`` skip fetching C++ dependencies from remote sources. ``OFF`` by default. All dependencies should be installed on the system already.

Running the unit tests
----------------------

After editing C++ code or tests, make sure to update the build/install:

.. code-block:: bash

  pixi run build

Alternatively, the ``all-tests`` CMake target can be used to build all tests.

There are two ways of running C++ tests.
Executables for the unit tests can be found in the build directory as ``build/bin/scipp-XYZ-test``, where ``XYZ`` is the Scipp component under test (e.g. ``core``).
These use ``google-test`` and provide full control over options, e.g., to filter tests:

.. code-block:: bash

   ./build/bin/scipp-common-test
   ./build/bin/scipp-core-test
   ./build/bin/scipp-units-test
   ./build/bin/scipp-variable-test
   ./build/bin/scipp-dataset-test

Alternatively, use ``ctest``:

.. code-block:: bash

  pixi run -- ctest --preset test

If only Python code or tests have been updated, there is no need to rebuild or reinstall, provided that you use an editable install (using ``pixi run editable`` as described earlier).

To run the Python tests, run (in the top-level directory):

.. code-block:: bash

  pixi run test


Building Documentation
----------------------

Run

.. code-block:: bash

  pixi run docs


This will build the HTML documentation and put it in a folder named ``html``.
If you want to build all docs after cleaning ``html`` and ``doctrees`` folders, please use ``pixi run docs-clean``.


Using Scipp as a C++ library
----------------------------

Using Scipp as a C++ library is not recommended at this point as the API (and ABI) is not stable and documentation is sparse.
Nonetheless, it can be used as a ``cmake`` package as follows.
In your ``CMakeLists.txt``:

.. code-block:: cmake

  # replace 23.01 with required version
  find_package(scipp 23.01 REQUIRED)

  target_link_libraries(mytarget PUBLIC scipp::dataset)

If Scipp was install using ``conda``, ``cmake`` should find it automatically.
If you build and installed Scipp from source use, e.g.,:

.. code-block:: bash

  cmake -DCMAKE_PREFIX_PATH=<your_scipp_install_dir>

where ``<your_scipp_install_dir>`` should point to the ``CMAKE_INSTALL_PREFIX`` that was used when building Scipp.
Alternative set the ``Scipp_DIR`` or ``CMAKE_PREFIX_PATH`` (environment) variables to this path.


Generating coverage reports
---------------------------

- Run ``cmake`` with options ``-DCOVERAGE=On -DCMAKE_BUILD_TYPE=Debug``.
- Run ``cmake --build . --target coverage`` from your build directory.
- Open ``coverage/index.html`` in a browser.

Build a wheel without CPM
-------------------------

If you want to build a wheel without pulling in C++ dependencies from remote sources via CPM, you can set the ``SKIP_REMOTE_SOURCES``
environment variable to ``true`` and it will use the system installed dependencies to build the wheels.
This is useful if you want to avoid network calls to CPM and build the wheels locally.

.. code-block:: bash

  SKIP_REMOTE_SOURCES=true python -m build

Note that setting this environment variable will automatically add the ``-DSKIP_REMOTE_SOURCES=ON`` CMake flag mentioned in `Additional build options`_.
