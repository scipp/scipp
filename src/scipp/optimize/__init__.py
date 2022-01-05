# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for optimization such as curve fitting.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.optimize`.
"""

from ..core import scalar, stddevs, Variable, DataArray
from ..core import BinEdgeError
from ..interpolate import _drop_masked
import numpy as np

from numbers import Real
from typing import Callable, List, Tuple, Union
from inspect import signature


def _as_scalar(obj, unit):
    if unit is None:
        return obj
    return scalar(value=obj, unit=unit)


def _wrap_func(f, p_units):
    def func(x, *args):
        p = [_as_scalar(v, u) for v, u in zip(args, p_units)]
        return f(x, *p).values

    return func


def _covariance_with_units(pcov, units):
    pcov = pcov.astype(dtype=object)
    for i, row in enumerate(pcov):
        for j, elem in enumerate(row):
            ui = units[i]
            uj = units[j]
            u = ui
            if u is None:
                u = uj
            elif uj is not None:
                u = ui * uj
            pcov[i, j] = _as_scalar(elem, u)
    return pcov


def curve_fit(
    f: Callable,
    da: DataArray,
    *,
    p0: List[Variable] = None,
    **kwargs
) -> Tuple[List[Union[Variable, Real]], List[List[Union[Variable, Real]]]]:
    """Use non-linear least squares to fit a function, f, to data.

    This is a wrapper around :py:class:`scipy.optimize.curve_fit`. See there for a
    complete description of parameters. The differences are:

    - Instead of separate xdata, ydata, and sigma arguments, the input data array
      defines provides these, with sigma defined as the square root of the variances,
      if present, i.e., the standard deviations.
    - The fit function f must work with scipp objects. This provides additional safety
      over the underlying scipy function by ensuring units are consistent.
    - The fit parameters may be scalar scipp variables. In that case an initial guess
      p0 with the correct units must be provided.
    - The returned optimal parameter values popt and the coverance matrix pcov will
      have units provided that the initial parameters have units.

    :param f: The model function, f(x, ...). It must take the independent variable
        (coordinate of the data array da) as the first argument and the parameters
        to fit as separate remaining arguments.
    :param da: One-dimensional data array. The dimension coordinate for the only
        dimension defines the independent variable where the data is measured. The
        values of the data array provide the dependent data. If the data array stores
        variances then the standard deviations (square root of the variances) are taken
        into account when fitting.
    :param p0: Initial guess for the parameters (length N). If None, then the initial
        values will all be 1 (if the number of parameters for the function can be
        determined using introspection, otherwise a ValueError is raised). If the fit
        function cannot handle initial values of 1, in particular for parameters that
        are not dimensionless, then typically a UnitError is raised, but details will
        depend on the function.

    Example:

      >>> def func(x, a, b):
      ...     return a * sc.exp(-b * x)

      >>> x = sc.linspace(dim='x', start=0.0, stop=0.4, num=50, unit='m')
      >>> y = func(x, a=5, b=17/sc.Unit('m'))
      >>> y.values += 0.01 * np.random.default_rng().normal(size=50)
      >>> da = sc.DataArray(y, coords={'x': x})

      >>> from scipp.optimize import curve_fit
      >>> popt, _ = curve_fit(func, da, p0 = [1.0, 1.0 / sc.Unit('m')])
      >>> round(popt[0])
      5
      >>> sc.round(popt[1])
      <scipp.Variable> ()    float64            [1/m]  [17.000000]
    """
    for arg in ['xdata', 'ydata', 'sigma']:
        if arg in kwargs:
            raise TypeError(
                f"Invalid argument '{arg}', already defined by the input data array.")
    if da.sizes[da.dim] != da.coords[da.dim].sizes[da.dim]:
        raise BinEdgeError("Cannot fit data array with bin-edge coordinate.")
    import scipy.optimize as opt
    da = _drop_masked(da, da.dim)
    sigma = stddevs(da).values if da.variances is not None else None
    x = da.coords[da.dim]
    sig = signature(f)
    if p0 is None:
        p0 = [1.0] * (len(sig.parameters) - 1)
    p_units = [p.unit if isinstance(p, Variable) else None for p in p0]
    p0 = [p.value if isinstance(p, Variable) else p for p in p0]
    popt, pcov = opt.curve_fit(f=_wrap_func(f, p_units),
                               xdata=x,
                               ydata=da.values,
                               sigma=sigma,
                               p0=p0)
    popt = np.array([_as_scalar(v, u) for v, u in zip(popt, p_units)])
    pcov = _covariance_with_units(pcov, p_units)
    return popt, pcov


__all__ = ['curve_fit']
