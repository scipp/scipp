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

  # Install pixi (if not already installed)
  curl -fsSL https://pixi.sh/install.sh | bash

  # Install pre-commit hooks
  pixi run -e lint static

  # Build and run tests
  pixi run test

You should now also be able to run ``pixi run import-minimal``.

Common tasks
~~~~~~~~~~~~

All of the below assume that the standard setup outlined above was performed.

Run the Python tests:

.. code-block:: bash

   pixi run test

Build the docs:

.. code-block:: bash

   pixi run docs

   # To clean and build all docs
   pixi run docs-clean

Run type checking:

.. code-block:: bash

   pixi run mypy

Run pre-commit checks:

.. code-block:: bash

   pixi run static

Overview
--------

We use `pixi <https://pixi.sh>`_ to manage all development dependencies and tasks.
A single ``pixi.toml`` file defines all environments, dependencies, and tasks.
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

For most development tasks, pixi handles building automatically.
When you run ``pixi run test`` or ``pixi run docs``, pixi-build-cmake builds the C++ extension
and installs it as a conda package in the environment.

For **incremental C++ development** (editing C++ code and rebuilding quickly), use the ``dev`` environment:

.. code-block:: bash

  # Configure, build, and run C++ tests
  pixi run -e dev cpp-test

  # Or step by step:
  pixi run -e dev configure
  pixi run -e dev build

The dev environment uses cmake presets (``ci-linux``, ``ci-macos``, ``ci-windows``) and installs
into per-environment directories under ``install/``.

To use the ``scipp`` Python module from a dev build:

.. code-block:: bash

  pixi shell -e dev
  python -c 'import scipp as sc'

The dev environment sets ``PYTHONPATH`` to the install directory automatically.

When making changes to the C++ side of Scipp, rebuild with:

.. code-block:: bash

  pixi run -e dev build


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

C++ tests (requires the ``dev`` environment):

.. code-block:: bash

  pixi run -e dev cpp-test

Python tests:

.. code-block:: bash

  pixi run test

Multi-Python version testing:

.. code-block:: bash

  pixi run -e test-py312 test
  pixi run -e test-py313 test
  pixi run -e test-py314 test

Building Documentation
----------------------

Run

.. code-block:: bash

  pixi run docs


This will build the HTML documentation and put it in a folder named ``html``.
If you want to build all docs after cleaning ``html`` and ``doctrees`` folders, use ``pixi run docs-clean``.


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
