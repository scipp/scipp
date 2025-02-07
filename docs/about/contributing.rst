.. _contributing:

Contributing to Scipp
=====================

Overview
--------

Contributions, bug reports, and ideas are always welcome.
The following section outlines the scope of Scipp.
If in doubt whether a feature falls within the scope of Scipp please `ask on github <https://github.com/scipp/scipp/issues>`_ before implementing functionality, to reduce the risk of rejected pull requests.
Asking and discussing first is generally always a good idea, since our road map is not very mature at this point.

Code of conduct
---------------

This project is a community effort, and everyone is welcome to contribute.
Everyone within the community is expected to abide by our `code of conduct <https://github.com/scipp/scipp/blob/main/CODE_OF_CONDUCT.md>`_.

Scope
-----

While Scipp is in practice developed for neutron-data reduction, the Scipp library itself must be kept generic.
We therefore restrict what can go into Scipp as follows:

* *Scipp* shall contain only generic functionality.
  Neutron-scattering specific must not be added.
  A simple guiding principle is "if it is in NumPy it can go into Scipp".

Contributing a pull request
---------------------------

Please make sure you have checked the list below before you start:

- Before you begin, make sure the GitHub issue is assigned to you, to avoid duplicate work.
- Read the `Getting Started <../development/getting-started.rst>`_ page.
  Using the recommended ``pre-commit`` setup and running tests locally will ensure that the builds will pass for your pull request.
- Reference the issue when you open a pull request.
  If the issue would be resolved by merging the pull request then use one of GitHub's keywords to do so automatically, such as ``Fixes #12345``.

Security
--------

Given the low (yet non-zero) chance of an issue in Scipp that affects the security of a larger system, security related issues should be raised via GitHub issues in the same way as "normal" bug reports.
