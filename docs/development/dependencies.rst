Dependencies
============

Version pinning
###############

Frequent issues such broken builds or inability to make a release have led us to freeze as many of our build dependencies as possible.
Note that this does *not* imply that runtime dependencies of the final packages are frozen.
There are a number of non-obvious dependencies that have led to surprising issues, so we attempt to document this below.

Places that define dependencies
###############################

- GitHub workflows define ``runs-on``, i.e., VM image versions.
  We are not using the ``*-latest`` images but instead rely on specific ones such as ``ubuntu-20.04`` or ``winddows-2019``.
  Unfortunately GitHub appears to make changes even to the non-latest images, e.g., we once experienced build breaks from a compiler update in the ``windows-2019`` images.
  In other words, the VM image "versions" are not truly frozen.
- Actions for GitHub workflows are frozen to specific versions.
  Dependabot is configured to update these regularely.
  Note one known issue:
  ``pypa/cibuildwheel`` updates may add support for new Python versions, but our ``pybind11`` may not be supporting it.
  Make sure to run the ``release.yml`` workflow before accepting an update, or consider adding future Python versions to the ``[tool.cibuildwheel]`` ``skip`` list to make support for new Python versions a process fully under our control.
- `pyproject.toml <https://github.com/scipp/scipp/blob/main/pyproject.toml>`_ defines build requirements for Python Wheels, runtime requirements, and "extras" for the ``pip`` package.
- All Python dependencies for static analysis, tests, documentation, and more are defined in ``requirements/*.in``.
  ``pip-compile-multi`` is used to generate the corresponding ``requirements/*.txt`` files.
  This should be run regularly, e.g., before a release.
  Create a PR with the results and *verify the changes*.
  This must include manual verification that the documentation is ok (download the artifact and inspect locally), since we have several Sphinx-related dependencies that have previously led to silently "broken" documentation pages.
- C++ conda build dependencies used in CI (non-release) builds are defined in `.buildconfig/ <https://github.com/scipp/scipp/tree/main/.buildconfig>`_ in files such as ``ci-linux.yml``.
  These need to be updated manually.
- `conda/meta.yaml <https://github.com/scipp/scipp/blob/main/conda/meta.yaml>`_ defines conda package build and runtime dependencies.
  Files in `conda/variants/ <https://github.com/scipp/scipp/tree/main/conda/variants>`_ complement this and set specific versions.
  These are currently not frozen explicitly.
  The goal is to move to the global conda-forge pin once they move to ``numpy-1.20``. See `#2571 <https://github.com/scipp/scipp/issues/2571>`_.
- Environment files in `docs/environments/ <https://github.com/scipp/scipp/tree/main/docs/environments>`_ list optional requirements.
  These environments are not used for builds but for users.

Services / Servers we depend on
###############################

This list is incomplete, but attempts to aid in better understanding how many things we depend on for making PR (including ``main``) and Release builds.

.. list-table:: Services
   :header-rows: 1
   :widths: 40 5 5 60

   * - Service
     - PR
     - Release
     - Comment
   * - `<https://github.com>`_
     - x
     - x
     -
   * - `<https://pypi.org>`_
     - x
     - x
     -
   * - `<https://anaconda.org/conda-forge>`_
     - x
     - x
     -
   * - `<https://public.esss.dk>`_
     - x
     - x
     - ``pooch`` files, accessed during ``docs`` build
