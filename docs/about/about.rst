.. _about:

About Scipp
===========

Version numbers and deprecation policy
--------------------------------------

Scipp uses `Calendar Versioning <https://calver.org/>`_.
The version number scheme is YY.0M.MICRO such as 22.11.0 or 23.02.0.
The final number is not the day but the micro (or patch) version and indicates "bugfix" releases without breaking or feature changes.

In contrast to semantic versioning, which may give a vague indirect hint about when breaking changes may occur, Scipp adopts the following explicit deprecation policy:

- Wherever possible, deprecation warnings will be added in releases before the actual change or removal of a feature.
  This starts the deprecation period.
- The length of the deprecation period will depend on the importance of the feature and on the user impact.

  - For major features this period might be 6 to 12 months, and we strive to include this in the deprecation announcement and in the deprecation warnings.
  - For minor features the period may be shorter and will generally not be announced explicitly.

- In some cases bugfixes can imply breaking changes.
  Scipp will generally not follow a deprecation process in these cases.
  That is, we prioritize correctness over convenience.
  In rare cases bugfix releases may thus break user code, or regular releases may break user code without prior deprecation warnings.
- Due to the calendar versioning scheme, deprecations can usually not name a concrete version for a removal.
  Instead, they name the earliest possible version.
  A message like "will be removed in version 23.09.0" should be read as "will be removed in the first version in or after September 2023".

Development
-----------

Scipp is a open source project by the `European Spallation Source ERIC <https://europeanspallationsource.se/>`_ (ESS).
Other major contributions were provided by the `Science and Technology Facilities Council <https://www.ukri.org/councils/stfc/>`_ (UKRI-STFC).

References and citations
------------------------

Please cite the following:

.. image:: https://zenodo.org/badge/147631466.svg
   :target: https://zenodo.org/badge/latestdoi/147631466

To cite a specific version of Scipp, select the desired version on Zenodo to get the corresponding DOI.

An article about Scipp has been published in the conference proceedings of ICANS, see `Scipp: Scientific data handling with labeled multi-dimensional arrays for C++ and Python <https://content.iospress.com/articles/journal-of-neutron-research/jnr190131>`_ or `the paywall-free copy on the Arxiv <https://arxiv.org/abs/2010.00257>`_.

A significant amount of changes such as multi-threading and support for binned (event) data has gone into Scipp since the publication so we recommend to also cite the Scipp documentation pages or version on Zenodo.

License
-------

Scipp is available as open source under the `BSD-3 license <https://opensource.org/licenses/BSD-3-Clause>`_.

Source code and development
---------------------------

Scipp is hosted and developed `on GitHub <https://github.com/scipp/scipp/projects>`_.
