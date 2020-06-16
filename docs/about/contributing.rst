.. _contributing:

Contributing to scipp
=====================

Overview
--------

Contributions, bug reports, and ideas are always welcome.
The following section outlines the scope of scipp.
If in doubt whether a feature falls within the scope of scipp please `ask on github <https://github.com/scipp/scipp/issues>`_ before implementing functionality, to reduce the risk of rejected pull requests.
Asking and discussing first is generally always a good idea, since our road map is not very mature at this point.

Scope
-----

While scipp is in practice developed for neutron-data reduction, the scipp library itself must be kept generic.
We therefore restrict what can go into scipp as follows:

* ``scipp`` shall contain only generic functionality.
  Neutron-scattering specific must not be added.
  A simple guiding principle is "if it is in ``numpy`` it can go into ``scipp``".

* ``scipp.neutron`` shall contain only generic neutron-specific functionality.
  Facility-specific or instrument-specific functionality must not be added.
  Examples of generic functionality that is permitted are 
  
  * Unit conversions, which could be generic for all time-of-flight neutron sources.
  * Published research such as absorption corrections.

  Examples of functionality that shall not be added to ``scipp.neutron`` are handling of facility-specific file types or data layouts, or instrument-specific correction algorithms.

Security
--------

Given the low (yet non-zero) chance of an issue in Scipp that affects the security of a larger system, secutity related issues should be raised via GitHub issues in the same way as "normal" bug reports.
