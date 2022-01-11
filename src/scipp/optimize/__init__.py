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
from typing import Callable, Dict, Tuple, Union
from inspect import getfullargspec


def _as_scalar(obj, unit):
    if unit is None:
        return obj
    return scalar(value=obj, unit=unit)


def _wrap_func(f, p_names, p_units):
    def func(x, *args):
        p = {k: _as_scalar(v, u) for k, v, u in zip(p_names, args, p_units)}
        return f(x, **p).values

    return func


def _covariance_with_units(p_names, pcov_values, units):
    pcov = {}
    for i, row in enumerate(pcov_values):
        pcov[p_names[i]] = {}
        for j, elem in enumerate(row):
            ui = units[i]
            uj = units[j]
            u = ui
            if u is None:
                u = uj
            elif uj is not None:
                u = ui * uj
            pcov[p_names[i]][p_names[j]] = _as_scalar(elem, u)
    return pcov


def _make_defaults(f, p0):
    spec = getfullargspec(f)
    if len(spec.args) != 1 or spec.varargs is not None:
        raise ValueError("Fit function must take exactly one positional argument")
    defaults = {} if spec.kwonlydefaults is None else spec.kwonlydefaults
    kwargs = {arg: 1.0 for arg in spec.kwonlyargs if arg not in defaults}
    if p0 is not None:
        kwargs.update(p0)
    return kwargs


def curve_fit(
    f: Callable,
    da: DataArray,
    *,
    p0: Dict[str, Variable] = None,
    **kwargs
) -> Tuple[Dict[str, Union[Variable, Real]], Dict[str, Dict[str, Union[Variable,
                                                                       Real]]]]:
    """Use non-linear least squares to fit a function, f, to data.

    This is a wrapper around :py:func:`scipy.optimize.curve_fit`. See there for a
    complete description of parameters. The differences are:

    - Instead of separate xdata, ydata, and sigma arguments, the input data array
      defines provides these, with sigma defined as the square root of the variances,
      if present, i.e., the standard deviations.
    - The fit function f must work with scipp objects. This provides additional safety
      over the underlying scipy function by ensuring units are consistent.
    - The fit function f must only take a single positional argument, x. All other
      arguments mapping to fit parameters must be keyword-only arguments.
    - The inital guess in p0 must be provided as a dict, mapping from fit-function
      parameter names to initial guesses.
    - The fit parameters may be scalar scipp variables. In that case an initial guess
      p0 with the correct units must be provided.
    - The returned optimal parameter values popt and the coverance matrix pcov will
      have units provided that the initial parameters have units. popt and pcov are
      a dict and a dict of dict, respectively. They are indexed using the fit parameter
      names. The variance of the returned optimal parameter values is set to the
      corresponding diagonal value of the covariance matrix.

    :param f: The model function, f(x, ...). It must take the independent variable
        (coordinate of the data array da) as the first argument and the parameters
        to fit as keyword arguments.
    :param da: One-dimensional data array. The dimension coordinate for the only
        dimension defines the independent variable where the data is measured. The
        values of the data array provide the dependent data. If the data array stores
        variances then the standard deviations (square root of the variances) are taken
        into account when fitting.
    :param p0: An optional dict of optional initial guesses for the parameters. If None,
        then the initial values will all be 1 (if the parameter names for the function
        can be determined using introspection, otherwise a ValueError is raised). If
        the fit function cannot handle initial values of 1, in particular for parameters
        that are not dimensionless, then typically a :py:class:`scipp.UnitError` is
        raised, but details will depend on the function.

    Example:

      >>> def func(x, *, a, b):
      ...     return a * sc.exp(-b * x)

      >>> x = sc.linspace(dim='x', start=0.0, stop=0.4, num=50, unit='m')
      >>> y = func(x, a=5, b=17/sc.Unit('m'))
      >>> y.values += 0.01 * np.random.default_rng().normal(size=50)
      >>> da = sc.DataArray(y, coords={'x': x})

      >>> from scipp.optimize import curve_fit
      >>> popt, _ = curve_fit(func, da, p0 = {'b': 1.0 / sc.Unit('m')})
      >>> sc.round(sc.values(popt['a']))
      <scipp.Variable> ()    float64            [dimensionless]  [5]
      >>> sc.round(sc.values(popt['b']))
      <scipp.Variable> ()    float64            [1/m]  [17]

    Fit-function parameters that have a default value do not participate in the fit
    unless an initial guess is provided via the p0 parameters:

      >>> from functools import partial
      >>> func2 = partial(func, a=5)
      >>> popt, _ = curve_fit(func2, da, p0 = {'b': 1.0 / sc.Unit('m')})
      >>> 'a' in popt
      False
      >>> popt, _ = curve_fit(func2, da, p0 = {'a':2, 'b': 1.0 / sc.Unit('m')})
      >>> 'a' in popt
      True
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
    p = _make_defaults(f, p0)
    p_units = [p.unit if isinstance(p, Variable) else None for p in p.values()]
    p0 = [p.value if isinstance(p, Variable) else p for p in p.values()]
    popt, pcov = opt.curve_fit(f=_wrap_func(f, p.keys(), p_units),
                               xdata=da.coords[da.dim],
                               ydata=da.values,
                               sigma=sigma,
                               p0=p0)
    popt = {
        name: scalar(value=val, variance=var, unit=u)
        for name, val, var, u in zip(p, popt, np.diag(pcov), p_units)
    }
    pcov = _covariance_with_units(list(p.keys()), pcov, p_units)
    return popt, pcov


__all__ = ['curve_fit']
