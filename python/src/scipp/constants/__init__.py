# flake8: noqa: E501
r"""
Physical and mathematical constants with units.
This module a wrapper around `scipy.constants <https://docs.scipy.org/doc/scipy/reference/constants.html>`_
and requires the ``scipy`` package to be installed.

Mathematical constants:

================  =================================================================
``pi``            Pi
``golden``        Golden ratio
``golden_ratio``  Golden ratio
================  =================================================================

Physical constants:

===========================  =============================================
``c``                        speed of light in vacuum
``speed_of_light``           speed of light in vacuum
``mu_0``                     the magnetic constant :math:`\mu_0`
``epsilon_0``                the electric constant (vacuum permittivity),
                             :math:`\epsilon_0`
``h``                        the Planck constant :math:`h`
``Planck``                   the Planck constant :math:`h`
``hbar``                     :math:`\hbar = h/(2\pi)`
``G``                        Newtonian constant of gravitation
``gravitational_constant``   Newtonian constant of gravitation
``g``                        standard acceleration of gravity
``e``                        elementary charge
``elementary_charge``        elementary charge
``R``                        molar gas constant
``gas_constant``             molar gas constant
``alpha``                    fine-structure constant
``fine_structure``           fine-structure constant
``N_A``                      Avogadro constant
``Avogadro``                 Avogadro constant
``k``                        Boltzmann constant
``Boltzmann``                Boltzmann constant
``sigma``                    Stefan-Boltzmann constant :math:`\sigma`
``Stefan_Boltzmann``         Stefan-Boltzmann constant :math:`\sigma`
``Wien``                     Wien displacement law constant
``Rydberg``                  Rydberg constant
``m_e``                      electron mass
``electron_mass``            electron mass
``m_p``                      proton mass
``proton_mass``              proton mass
``m_n``                      neutron mass
``neutron_mass``             neutron mass
===========================  =============================================

In addition to the above variables, :mod:`scipp.constants` also contains the
2018 CODATA recommended values [CODATA2018]_ database containing more physical
constants.
The database is accessed using :py:func:`scipp.constants.physical_constants`.

.. [CODATA2018] CODATA Recommended Values of the Fundamental
   Physical Constants 2018.
   https://physics.nist.gov/cuu/Constants/
"""
import math as _math
from .. import as_const, scalar, Variable


def physical_constants(key: str, with_variance: bool = False) -> Variable:
    """
    Returns the CODATA recommended value with unit of the requested physical constant.

    :param key: Key of the requested constant. See `scipy.constants.physical_constants <https://docs.scipy.org/doc/scipy/reference/constants.html#scipy.constants.physical_constants>`_ for an overview.
    :param with_variance: Optional, if True, the uncertainty if the constant is
                          included as the variance. Default is False.
    :returns: Scalar variable with unit and optional variance.
    :rtype: Variable
    """
    from scipy.constants import physical_constants as _cd
    value, unit, uncertainty = _cd[key]
    args = {'value': value, 'unit': unit.replace(' ', '*')}
    if with_variance:
        stddev = uncertainty
        args['variance'] = stddev * stddev
    return as_const(scalar(**args))


# mathematical constants
pi = scalar(_math.pi)
golden = golden_ratio = scalar((1 + _math.sqrt(5)) / 2)

# physical constants
c = speed_of_light = physical_constants('speed of light in vacuum')
mu_0 = physical_constants('vacuum mag. permeability')
epsilon_0 = physical_constants('vacuum electric permittivity')
h = Planck = physical_constants('Planck constant')
hbar = h / (2 * pi)
G = gravitational_constant = physical_constants('Newtonian constant of gravitation')
g = physical_constants('standard acceleration of gravity')
e = elementary_charge = physical_constants('elementary charge')
R = gas_constant = physical_constants('molar gas constant')
alpha = fine_structure = physical_constants('fine-structure constant')
N_A = Avogadro = physical_constants('Avogadro constant')
k = Boltzmann = physical_constants('Boltzmann constant')
sigma = Stefan_Boltzmann = physical_constants('Stefan-Boltzmann constant')
Wien = physical_constants('Wien wavelength displacement law constant')
Rydberg = physical_constants('Rydberg constant')

m_e = electron_mass = physical_constants('electron mass')
m_p = proton_mass = physical_constants('proton mass')
m_n = neutron_mass = physical_constants('neutron mass')
m_u = u = atomic_mass = physical_constants('atomic mass constant')
