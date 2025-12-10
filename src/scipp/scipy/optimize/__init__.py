# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
"""Sub-package for optimization such as curve fitting.

This subpackage provides wrappers for a subset of functions from
:py:mod:`scipy.optimize`.
"""

from collections.abc import Callable, Iterable, Mapping
from inspect import getfullargspec
from numbers import Real
from typing import Any, TypeVar

import numpy as np
import numpy.typing as npt

from ...core import (
    BinEdgeError,
    DataArray,
    DefaultUnit,
    Unit,
    Variable,
    scalar,
    stddevs,
)
from ...typing import VariableLike
from ...units import default_unit, dimensionless
from ..interpolate import _drop_masked

_VarDa = TypeVar('_VarDa', Variable, DataArray)


def _as_scalar(obj: Any, unit: Unit | DefaultUnit | None) -> Any:
    if unit == default_unit:
        return obj
    return scalar(
        value=obj,
        unit=unit,
    )


def _wrap_func(
    f: Callable[..., Variable | DataArray],
    p_names: Iterable[str],
    p_units: Iterable[Unit | DefaultUnit | None],
) -> Callable[..., npt.NDArray[Any]]:
    def func(x: VariableLike, *args: Any) -> npt.NDArray[Any]:
        p = {
            k: _as_scalar(v, u) for k, v, u in zip(p_names, args, p_units, strict=True)
        }
        return f(x, **p).values  # type: ignore[no-any-return]

    return func


def _get_sigma(
    da: _VarDa,
) -> npt.NDArray[np.float64 | np.float32] | None:
    if da.variances is None:
        return None

    sigma = stddevs(da).values
    if not sigma.all():
        raise ValueError(
            'There is a 0 in the input variances. This would break the optimizer. '
            'Mask the offending elements, remove them, or assign a meaningful '
            'variance if possible before calling curve_fit.'
        )
    return sigma  # type: ignore[no-any-return]


def _covariance_with_units(
    p_names: list[str],
    pcov_values: npt.NDArray[np.float64 | np.float32],
    units: list[Unit | DefaultUnit | None],
) -> dict[str, dict[str, Variable | Real]]:
    pcov: dict[str, dict[str, Variable | Real]] = {}
    for i, row in enumerate(pcov_values):
        pcov[p_names[i]] = {}
        for j, elem in enumerate(row):
            ui = units[i]
            uj = units[j]
            u = ui
            if u == default_unit:
                u = uj
            elif uj != default_unit:
                u = ui * uj  # type: ignore[assignment, operator]  # neither operand is DefaultUnit
            pcov[p_names[i]][p_names[j]] = _as_scalar(elem, u)
    return pcov


def _make_defaults(
    f: Callable[..., Any], p0: Mapping[str, Variable | float] | None
) -> dict[str, Any]:
    spec = getfullargspec(f)
    if len(spec.args) != 1 or spec.varargs is not None:
        raise ValueError("Fit function must take exactly one positional argument")
    defaults = {} if spec.kwonlydefaults is None else spec.kwonlydefaults
    kwargs: dict[str, Any] = {
        arg: 1.0 for arg in spec.kwonlyargs if arg not in defaults
    }
    if p0 is not None:
        kwargs.update(p0)
    return kwargs


def _get_specific_bounds(
    bounds: Mapping[str, tuple[Variable, Variable] | tuple[float, float]],
    name: str,
    unit: Unit | None,
) -> tuple[float, float]:
    if name not in bounds:
        return -np.inf, np.inf
    b = bounds[name]
    if len(b) != 2:
        raise ValueError(
            "Parameter bounds must be given as a tuple of length 2. "
            f"Got a collection of length {len(b)} as bounds for '{name}'."
        )
    if isinstance(b[0], Variable):
        return (
            b[0].to(unit=unit, dtype=float).value,
            b[1].to(unit=unit, dtype=float).value,
        )
    return b


def _parse_bounds(
    bounds: Mapping[str, tuple[Variable, Variable] | tuple[float, float]] | None,
    params: Mapping[str, Any],
) -> (
    tuple[float, float]
    | tuple[npt.NDArray[np.float64 | np.float32], npt.NDArray[np.float64 | np.float32]]
):
    if bounds is None:
        return -np.inf, np.inf

    bounds_tuples = [
        _get_specific_bounds(
            bounds, name, param.unit if isinstance(param, Variable) else dimensionless
        )
        for name, param in params.items()
    ]
    bounds_array = np.array(bounds_tuples).T
    return bounds_array[0], bounds_array[1]


def curve_fit(
    f: Callable[..., Variable | DataArray],
    da: DataArray,
    *,
    p0: Mapping[str, Variable | float] | None = None,
    bounds: Mapping[str, tuple[Variable, Variable] | tuple[float, float]] | None = None,
    **kwargs: Any,
) -> tuple[dict[Any, Any], dict[str, dict[str, Variable | Real]]]:
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
    - The initial guess in p0 must be provided as a dict, mapping from fit-function
      parameter names to initial guesses.
    - The parameter bounds must also be provided as a dict, like p0.
    - The fit parameters may be scalar scipp variables. In that case an initial guess
      p0 with the correct units must be provided.
    - The returned optimal parameter values popt and the coverance matrix pcov will
      have units provided that the initial parameters have units. popt and pcov are
      a dict and a dict of dict, respectively. They are indexed using the fit parameter
      names. The variance of the returned optimal parameter values is set to the
      corresponding diagonal value of the covariance matrix.

    Parameters
    ----------
    f:
        The model function, f(x, ...). It must take the independent variable
        (coordinate of the data array da) as the first argument and the parameters
        to fit as keyword arguments.
    da:
        One-dimensional data array. The dimension coordinate for the only
        dimension defines the independent variable where the data is measured. The
        values of the data array provide the dependent data. If the data array stores
        variances then the standard deviations (square root of the variances) are taken
        into account when fitting.
    p0:
        An optional dict of optional initial guesses for the parameters. If None,
        then the initial values will all be 1 (if the parameter names for the function
        can be determined using introspection, otherwise a ValueError is raised). If
        the fit function cannot handle initial values of 1, in particular for parameters
        that are not dimensionless, then typically a :py:class:`scipp.UnitError` is
        raised, but details will depend on the function.
    bounds:
        Lower and upper bounds on parameters.
        Defaults to no bounds.
        Bounds are given as a dict of 2-tuples of (lower, upper) for each parameter
        where lower and upper are either both Variables or plain numbers.
        Parameters omitted from the `bounds` dict are unbounded.

    Returns
    -------
    popt:
        Optimal values for the parameters.
    pcov:
        The estimated covariance of popt.

    See Also
    --------
    scipp.curve_fit:
        Supports N-D curve fits.

    Examples
    --------

      >>> def func(x, *, a, b):
      ...     return a * sc.exp(-b * x)

      >>> x = sc.linspace(dim='x', start=0.0, stop=0.4, num=50, unit='m')
      >>> y = func(x, a=5, b=17/sc.Unit('m'))
      >>> y.values += 0.01 * np.random.default_rng().normal(size=50)
      >>> da = sc.DataArray(y, coords={'x': x})

      >>> from scipp.scipy.optimize import curve_fit
      >>> popt, _ = curve_fit(func, da, p0 = {'b': 1.0 / sc.Unit('m')})
      >>> sc.round(sc.values(popt['a']))
      <scipp.Variable> ()    float64            [dimensionless]  5
      >>> sc.round(sc.values(popt['b']))
      <scipp.Variable> ()    float64            [1/m]  17

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
    if 'jac' in kwargs:
        raise NotImplementedError(
            "The 'jac' argument is not yet supported. "
            "See https://github.com/scipp/scipp/issues/2544"
        )
    for arg in ['xdata', 'ydata', 'sigma']:
        if arg in kwargs:
            raise TypeError(
                f"Invalid argument '{arg}', already defined by the input data array."
            )
    if da.sizes[da.dim] != da.coords[da.dim].sizes[da.dim]:
        raise BinEdgeError("Cannot fit data array with bin-edge coordinate.")
    import scipy.optimize as opt

    da = _drop_masked(da, da.dim)
    params = _make_defaults(f, p0)
    p_units = [
        p.unit if isinstance(p, Variable) else default_unit for p in params.values()
    ]
    p0_values = [p.value if isinstance(p, Variable) else p for p in params.values()]
    popt, pcov = opt.curve_fit(
        f=_wrap_func(f, params.keys(), p_units),
        xdata=da.coords[da.dim],
        ydata=da.values,
        sigma=_get_sigma(da),
        p0=p0_values,
        bounds=_parse_bounds(bounds, params),
        **kwargs,
    )
    popt = {
        name: scalar(value=val, variance=var, unit=u)
        for name, val, var, u in zip(
            params.keys(), popt, np.diag(pcov), p_units, strict=True
        )
    }
    pcov = _covariance_with_units(list(params.keys()), pcov, p_units)
    return popt, pcov


__all__ = ['curve_fit']
