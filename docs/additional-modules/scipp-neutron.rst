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

    +--------------+--------------+-----------------------------------------------------------------------------------------------------------+
    |From (unit)   |To (unit)     |Formula                                                                                                    |
    +==============+==============+===========================================================================================================+
    |``Tof``       |``DSpacing``  |:math:`d = \frac{\lambda}{2sin(\theta)} = \frac{h \times tof}{L_{total} \times m_{n} \times 2 sin(\theta)}`|
    +--------------+--------------+-----------------------------------------------------------------------------------------------------------+
    |``DSpacing``  |``Tof``       |:math:`tof = \frac{d \times L_{total} \times m_{n} \times 2 sin(\theta)}{h}`                               |
    +--------------+--------------+-----------------------------------------------------------------------------------------------------------+
    |``Tof``       |``Wavelength``|:math:`\lambda = \frac{h \times tof}{m_{n} \times L_{total}}`                                              |
    +--------------+--------------+-----------------------------------------------------------------------------------------------------------+
    |``Wavelength``|``Tof``       |:math:`tof = \frac{m_{n} \times L_{total} \times \lambda}{h}`                                              |
    +--------------+--------------+-----------------------------------------------------------------------------------------------------------+
    |``Tof``       |``Energy``    |:math:`E = \frac{1}{2}m_{n}\left(\frac{L_{total}}{tof}\right)^{2}`                                         |
    +--------------+--------------+-----------------------------------------------------------------------------------------------------------+
    |``Energy``    |``Tof``       |:math:`tof = \frac{l_{total}}{\sqrt{\frac{2 E}{m_{n}}}}`                                                   |
    +--------------+--------------+-----------------------------------------------------------------------------------------------------------+

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
