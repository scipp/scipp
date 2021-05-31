Dependencies
============

Places where dependencies are specified:

  - Conda package build (`conda/meta.yml <https://github.com/scipp/scipp/blob/main/conda/meta.yaml>`_)
  - Developer environment (`scipp-developer.yml <https://github.com/scipp/scipp/blob/main/scipp-developer.yml>`_)

CMake ExternalProject
#####################

  - googlebenchmark (Apache-2.0) (microbenchmark framework)
  - googletest (BSD-3-Clause) (unit test framework)

Conda packages
##############

Those marked with "*" are optional.
Some functionality may be missing (e.g. some unit tests skipped) but core functionality is not changed.

Build
-----

  - ``tbb-devel`` (Apache-2.0) (Intel TBB headers, used for single node multithreading)

Run
---

  - ``appdirs`` (MIT) (system application directory helper, used in user configuration loading)
  - ``ipywidgets`` * (BSD-3-Clause) (interactive Jupyter notebook widgets)
  - ``matplotlib`` * (custom permissive) (plotting library)
  - ``ipympl`` * (BSD-3-Clause) (matplotlib-jupyter integration)
  - ``numpy`` >=1.15.3 (custom permissive)
  - ``python`` (PSF-2.0)
  - ``python-configuration`` (MIT) (configuration file parser, used in user configuration loading)
  - ``pythreejs`` * (BSD-3-Clause) (3D plotting library, used in plot_3d and instrument_view)
  - ``pyyaml`` (MIT) (YAML parser, used for ?)
  - ``tbb`` (Apache-2.0) (Intel TBB runtime, used for single node multithreading)

Test
----

  - all (including optional) dependencies of *Run*
  - ``pytest`` (MIT) (test framework)

Jupyter Lab
-----------
To use scipp with Jupyter Lab:

- ``jupyterlab`` (BSD-3-Clause)
- ``jupyterlab_widgets`` (BSD-3-Clause) (jupterlab widgets extension)
- ``nodejs`` (MIT) (javascript runtime)

Documentation Build
-------------------

  - all (including optional) dependecies of *Run*
  - ``ipython`` 7.2.0 (BSD-3-Clause) (used for ?)
  - ``pandoc`` (GPL-2.0-only) (used for ?)
  - ``sphinx`` >=1.6 (BSD-2-Clause) (documentation build framework)
  - ``sphinx_rtd_theme`` (MIT) (Sphinx theme)
  - ``nbsphinx`` (MIT) (Jupyter notebook Sphinx extension)
