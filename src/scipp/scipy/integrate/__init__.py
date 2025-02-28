# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for integration.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.integrate`.
"""

from collections.abc import Callable
from typing import Any

import numpy.typing as npt

from ...compat.wrapping import wrap1d
from ...core import DataArray, Unit, array


def _integrate(
    func: Callable[..., npt.NDArray[Any]], da: DataArray, dim: str, **kwargs: Any
) -> DataArray:
    if 'dx' in kwargs:
        raise ValueError(
            "Invalid argument 'dx': Spacing of integration points is "
            f"given by the '{dim}' coord."
        )
    integral = func(x=da.coords[dim].values, y=da.values, **kwargs)
    dims = [d for d in da.dims if d != dim]
    unit: Unit = da.unit * da.coords[dim].unit  # type: ignore[assignment, operator]  # from unit = None
    return DataArray(data=array(dims=dims, values=integral, unit=unit))


@wrap1d()
def trapezoid(da: DataArray, dim: str, **kwargs: Any) -> DataArray:
    """Integrate data array along the given dimension with
    the composite trapezoidal rule.

    This is a wrapper around :py:func:`scipy.integrate.trapezoid`.

    Examples:

      >>> x = sc.geomspace(dim='x', start=0.1, stop=0.4, num=4, unit='m')
      >>> da = sc.DataArray(x*x, coords={'x': x})
      >>> from scipp.scipy.integrate import trapezoid
      >>> trapezoid(da, 'x')
      <scipp.DataArray>
      Dimensions: Sizes[]
      Data:
                                  float64            [m^3]  ()  0.0217094
    """
    import scipy.integrate as integ

    return _integrate(integ.trapezoid, da, dim, **kwargs)


@wrap1d()
def simpson(da: DataArray, dim: str, **kwargs: Any) -> DataArray:
    """Integrate data array along the given dimension with the composite Simpson's rule.

    This is a wrapper around :py:func:`scipy.integrate.simpson`.

    Examples:

      >>> x = sc.geomspace(dim='x', start=0.1, stop=0.4, num=4, unit='m')
      >>> da = sc.DataArray(x*x, coords={'x': x})
      >>> from scipp.scipy.integrate import simpson
      >>> simpson(da, 'x')
      <scipp.DataArray>
      Dimensions: Sizes[]
      Data:
                                  float64            [m^3]  ()  0.021
    """
    import scipy.integrate as integ

    return _integrate(integ.simpson, da, dim, **kwargs)


__all__ = ['simpson', 'trapezoid']
