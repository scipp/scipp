# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for integration.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.integrate`.
"""

from ..core import array, DataArray
from ..compat.wrapping import wrap1d

from typing import Callable


def _integrate(func: Callable, da: DataArray, dim: str, **kwargs) -> DataArray:
    if 'dx' in kwargs:
        raise ValueError("Invalid argument 'dx': Spacing of integration points is "
                         f"given by the '{dim}' coord.")
    integral = func(x=da.coords[dim].values, y=da.values, **kwargs)
    dims = [d for d in da.dims if d != dim]
    return DataArray(
        data=array(dims=dims, values=integral, unit=da.unit * da.coords[dim].unit))


@wrap1d()
def trapezoid(da: DataArray, dim: str, **kwargs) -> DataArray:
    """Integrate data array along the given dimension with the composite trapezoidal rule.

    This is a wrapper around :py:func:`scipy.integrate.trapezoid`.

    Examples:

      >>> x = sc.geomspace(dim='x', start=0.1, stop=0.4, num=4, unit='m')
      >>> da = sc.DataArray(x*x, coords={'x': x})
      >>> from scipp.integrate import trapezoid
      >>> trapezoid(da, 'x')
      <scipp.DataArray>
      Dimensions: Sizes[]
      Data:
                                  float64            [m^3]  ()  [0.0217094]
    """
    import scipy.integrate as integ
    return _integrate(integ.trapezoid, da, dim, **kwargs)


@wrap1d()
def simpson(da: DataArray, dim: str, **kwargs) -> DataArray:
    """Integrate data array along the given dimension with the composite Simpson's rule.

    This is a wrapper around :py:func:`scipy.integrate.simpson`.

    Examples:

      >>> x = sc.geomspace(dim='x', start=0.1, stop=0.4, num=4, unit='m')
      >>> da = sc.DataArray(x*x, coords={'x': x})
      >>> from scipp.integrate import simpson
      >>> simpson(da, 'x')
      <scipp.DataArray>
      Dimensions: Sizes[]
      Data:
                                  float64            [m^3]  ()  [0.0212871]
    """
    import scipy.integrate as integ
    return _integrate(integ.simpson, da, dim, **kwargs)


__all__ = ['simpson', 'trapezoid']
