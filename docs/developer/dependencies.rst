Dependencies
============

Places where dependencies are specified:

  - Conda package build (`conda/meta.yml <https://github.com/scipp/scipp/blob/master/conda/meta.yaml>`_)
  - Documentation build environment (`docs/environment.yml <https://github.com/scipp/scipp/blob/master/docs/environment.yml>`_)

Conda packages
##############

Those marked with "*" are optional.
SOme functionality may be missing (e.g. some unit tests skipped) but core functionality is not changed.

Build
-----

  - ``tbb-devel`` (Intel TBB headers, used for single node multithreading)

Run
---

  - ``appdirs`` (system application directory helper, used in user configuration loading)
  - ``beautifulsoup4`` * (HTML parser, used for ?)
  - ``ipyevents`` *
  - ``ipyvolume`` *
  - ``ipywidgets`` *
  - ``mantid-framework`` * (used for Mantid interoperability)
  - ``matplotlib`` * (plotting library)
  - ``numpy >=1.15.3``
  - ``python``
  - ``python-configuration`` (configuration file parser, used in user configuration loading)
  - ``pyyaml`` (YAML parser, used for ?)
  - ``tbb`` (Intel TBB runtime, used for single node multithreading)

Test
----

  - all (including optional) dependecies of *Run*
  - ``mantid-framework`` * (Mantid interoperability tests are skipped if not present)
  - ``pytest`` (test framework)

Documentation Build
-------------------

  - all (including optional) dependecies of *Run*
  - ``ipython=7.2.0`` (used for ?)
  - ``pip`` (used for ?)
  - ``pandoc`` (used for ?)
  - ``sphinx>=1.6`` (documentation build framework)
  - ``sphinx_rtd_theme`` (Sphinx theme)
  - ``nbsphinx`` (Jupyter notebook Sphinx extension)
