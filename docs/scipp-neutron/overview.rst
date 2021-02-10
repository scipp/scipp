.. _scipp-neutron:
.. currentmodule:: scipp.neutron

Overview: scipp.neutron
=======================

``scipp-neutron`` builds on top of the core ``scipp`` package and provides features specific to handling data generated in neutron scattering facilities.
A key example is "unit conversion", e.g., from time-of-flight to energy transfer in an inelastic neutron scattering experiment at a spallation-based neutron source.

Free functions
--------------

Mantid Compatibility
~~~~~~~~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   from_mantid
   to_mantid
   fit

Unit Conversion
~~~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   convert

Beamline geometry
~~~~~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   source_position
   sample_position
   l1
   l2
   scattering_angle
   two_theta

Loading Nexus files
~~~~~~~~~~~~~~~~~~~

.. autosummary::
   :toctree: ../generated

   load
