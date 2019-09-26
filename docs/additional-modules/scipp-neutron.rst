.. _scipp-neutron:
.. currentmodule:: scipp.neutron

scipp-neutron
==============

``scipp-neutron`` builds on top of ``scipp-core`` and provides features specific to handling data generated in neutron scattering facilities.
A key example is "unit conversion", e.g., from time-of-flight to energy transfer in an inelastic neutron scattering experiment at a spallation-based neutron source.

Unit Conversions
----------------

The table below shows the supported unit conversions.

.. table::

    +--------------+--------------+----------------------------------------------------------------------------------------------------------------------------------------+
    |From (unit)   |To (unit)     |Formula                                                                                                                                 |
    +==============+==============+========================================================================================================================================+
    |``Tof``       |``DSpacing``  |:math:`d = \frac{\lambda}{2\sin(\theta)} = \frac{h \times \mathrm{tof}}{L_{\mathrm{total}} \times m_{\mathrm{n}} \times 2 \sin(\theta)}`|
    +--------------+--------------+----------------------------------------------------------------------------------------------------------------------------------------+
    |``DSpacing``  |``Tof``       |:math:`\mathrm{tof} = \frac{d \times L_{\mathrm{total}} \times m_{\mathrm{n}} \times 2 \sin(\theta)}{h}`                                |
    +--------------+--------------+----------------------------------------------------------------------------------------------------------------------------------------+
    |``Tof``       |``Wavelength``|:math:`\lambda = \frac{h \times \mathrm{tof}}{m_{\mathrm{n}} \times L_{\mathrm{total}}}`                                                |
    +--------------+--------------+----------------------------------------------------------------------------------------------------------------------------------------+
    |``Wavelength``|``Tof``       |:math:`\mathrm{tof} = \frac{m_{\mathrm{n}} \times L_{\mathrm{total}} \times \lambda}{h}`                                                |
    +--------------+--------------+----------------------------------------------------------------------------------------------------------------------------------------+
    |``Tof``       |``Energy``    |:math:`E = \frac{1}{2}m_{\mathrm{n}}\left(\frac{L_{\mathrm{total}}}{\mathrm{tof}}\right)^{2}`                                           |
    +--------------+--------------+----------------------------------------------------------------------------------------------------------------------------------------+
    |``Energy``    |``Tof``       |:math:`\mathrm{tof} = \frac{L_{\mathrm{total}}}{\sqrt{\frac{2 E}{m_{\mathrm{n}}}}}`                                                     |
    +--------------+--------------+----------------------------------------------------------------------------------------------------------------------------------------+

Free functions
--------------

Conversions
~~~~~~~~~~~

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
