# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)

from collections.abc import Callable, Mapping, Sequence
from inspect import getfullargspec
from numbers import Real

import numpy as np

from .core import (
    BinEdgeError,
    DataArray,
    DataGroup,
    DimensionError,
    Variable,
    array,
    irreducible_mask,
    scalar,
    stddevs,
)


def _wrap_scipp_func(f, p0):
    p = {k: scalar(0.0, unit=v.unit) for k, v in p0.items()}

    def func(x, *args):
        for k, v in zip(p, args, strict=True):
            p[k].value = v
        return f(**x, **p).values

    return func


def _wrap_numpy_func(f, param_names, coord_names):
    def func(x, *args):
        # If there is only one predictor variable x might be a 1D array.
        # Make x 2D for consistency.
        if len(x.shape) == 1:
            x = x.reshape(1, -1)
        c = dict(zip(coord_names, x, strict=True))
        p = dict(zip(param_names, args, strict=True))
        return f(**c, **p)

    return func


def _get_sigma(da):
    if da.variances is None:
        return None

    sigma = stddevs(da).values
    if not sigma.all():
        raise ValueError(
            'There is a 0 in the input variances. This would break the optimizer. '
            'Mask the offending elements, remove them, or assign a meaningful '
            'variance if possible before calling curve_fit.'
        )
    return sigma


def _datagroup_outputs(da, p0, map_over, pdata, covdata):
    variances = np.diagonal(covdata, axis1=-2, axis2=-1)
    dg = DataGroup(
        {
            p: DataArray(
                data=array(
                    dims=map_over,
                    values=pdata[..., i] if pdata.ndim > 1 else pdata[i],
                    variances=variances[..., i] if variances.ndim > 1 else variances[i],
                    unit=v0.unit,
                ),
            )
            for i, (p, v0) in enumerate(p0.items())
        }
    )
    dgcov = DataGroup(
        {
            p: DataGroup(
                {
                    q: DataArray(
                        data=array(
                            dims=map_over,
                            values=covdata[..., i, j]
                            if covdata.ndim > 2
                            else covdata[i, j],
                            unit=v0.unit * u0.unit,
                        ),
                    )
                    for j, (q, u0) in enumerate(p0.items())
                }
            )
            for i, (p, v0) in enumerate(p0.items())
        }
    )
    for c in da.coords:
        if set(map_over).intersection(da.coords[c].dims):
            for p in dg:
                dg[p].coords[c] = da.coords[c]
                for q in dgcov[p]:
                    dgcov[p][q].coords[c] = da.coords[c]
    for m in da.masks:
        # Drop masks that don't fit the output data
        if set(da.masks[m].dims).issubset(set(dg.dims)):
            for p in dg:
                dg[p].masks[m] = da.masks[m]
                for q in dgcov[p]:
                    dgcov[p][q].masks[m] = da.masks[m]
    return dg, dgcov


def _prepare_numpy_outputs(da, p0, map_over):
    shape = [da.sizes[d] for d in map_over]
    dg = np.empty([*shape, len(p0)])
    dgcov = np.empty(shape + 2 * [len(p0)])
    return dg, dgcov


def _make_defaults(f, coords, p0):
    spec = getfullargspec(f)
    all_args = {*spec.args, *spec.kwonlyargs}
    if not set(coords).issubset(all_args):
        raise ValueError("Function must take the provided coords as arguments")
    default_arguments = dict(
        zip(spec.args[-len(spec.defaults) :], spec.defaults, strict=True)
        if spec.defaults
        else {},
        **(spec.kwonlydefaults or {}),
    )
    return {
        **{a: scalar(1.0) for a in all_args - set(coords)},
        **default_arguments,
        **{
            k: v if isinstance(v, Variable) else scalar(v)
            for k, v in (p0 or {}).items()
        },
    }


def _get_specific_bounds(bounds, name, unit):
    if name not in bounds:
        return -scalar(np.inf, unit=unit), scalar(np.inf, unit=unit)
    b = bounds[name]
    if len(b) != 2:
        raise ValueError(
            "Parameter bounds must be given as a tuple of length 2. "
            f"Got a collection of length {len(b)} as bounds for '{name}'."
        )
    if (
        b[0] is not None
        and b[1] is not None
        and isinstance(b[0], Variable) ^ isinstance(b[1], Variable)
    ):
        raise ValueError(
            f"Bounds cannot mix Scipp variables and other number types, "
            f"got {type(b[0])} and {type(b[1])}"
        )
    le = -scalar(np.inf, unit=unit) if b[0] is None else b[0]
    ri = scalar(np.inf, unit=unit) if b[1] is None else b[1]
    le, ri = (
        v.to(unit=unit) if isinstance(v, Variable) else scalar(v).to(unit=unit)
        for v in (le, ri)
    )
    return le, ri


def _parse_bounds(bounds, p0):
    return {k: _get_specific_bounds(bounds or {}, k, v.unit) for k, v in p0.items()}


def _reshape_bounds(bounds):
    left, right = zip(*bounds.values(), strict=True)
    left, right = [le.value for le in left], [ri.value for ri in right]
    if all(le == -np.inf and ri == np.inf for le, ri in zip(left, right, strict=True)):
        return -np.inf, np.inf
    return left, right


def _curve_fit(
    f,
    da,
    p0,
    bounds,
    map_over,
    unsafe_numpy_f,
    out,
    **kwargs,
):
    dg, dgcov = out

    if len(map_over) > 0:
        dim = map_over[0]
        for i in range(da.sizes[dim]):
            _curve_fit(
                f,
                da[dim, i],
                {k: v[dim, i] if dim in v.dims else v for k, v in p0.items()},
                {
                    k: (
                        le[dim, i] if dim in le.dims else le,
                        ri[dim, i] if dim in ri.dims else ri,
                    )
                    for k, (le, ri) in bounds.items()
                },
                map_over[1:],
                unsafe_numpy_f,
                (dg[i], dgcov[i]),
                **kwargs,
            )

        return

    for k, v in p0.items():
        if v.shape != ():
            raise DimensionError(f'Parameter {k} has unexpected dimensions {v.dims}')

    for k, (le, ri) in bounds.items():
        if le.shape != ():
            raise DimensionError(
                f'Left bound of parameter {k} has unexpected dimensions {le.dims}'
            )
        if ri.shape != ():
            raise DimensionError(
                f'Right bound of parameter {k} has unexpected dimensions {ri.dims}'
            )

    fda = da.flatten(to='row')
    mask = irreducible_mask(fda.masks, 'row')
    if mask is not None:
        fda = fda[~mask]

    if not unsafe_numpy_f:
        # Making the coords into a dict improves runtime,
        # probably because of pybind overhead.
        X = dict(fda.coords)
    else:
        X = np.vstack([c.values for c in fda.coords.values()], dtype='float')

    import scipy.optimize as opt

    if len(fda) < len(dg):
        # More parameters than data points, unable to fit, abort.
        dg[:] = np.nan
        dgcov[:] = np.nan
        return

    try:
        popt, pcov = opt.curve_fit(
            f,
            X,
            fda.data.values,
            [v.value for v in p0.values()],
            sigma=_get_sigma(fda),
            bounds=_reshape_bounds(bounds),
            **kwargs,
        )
    except RuntimeError as err:
        if hasattr(err, 'message') and 'Optimal parameters not found:' in err.message:
            popt = np.array([np.nan for p in p0])
            pcov = np.array([[np.nan for q in p0] for p in p0])
        else:
            raise err

    dg[:] = popt
    dgcov[:] = pcov


def curve_fit(
    coords: Sequence[str] | Mapping[str, str | Variable],
    f: Callable,
    da: DataArray,
    *,
    p0: dict[str, Variable | Real] | None = None,
    bounds: dict[str, tuple[Variable, Variable] | tuple[Real, Real]] | None = None,
    reduce_dims: Sequence[str] = (),
    unsafe_numpy_f: bool = False,
    **kwargs,
) -> tuple[DataGroup, DataGroup]:
    """Use non-linear least squares to fit a function, f, to data.
    The function interface is similar to that of :py:func:`xarray.DataArray.curvefit`.

    .. versionadded:: 23.12.0

    This is a wrapper around :py:func:`scipy.optimize.curve_fit`. See there for
    in depth documentation and keyword arguments. The differences are:

    - Instead of separate ``xdata``, ``ydata``, and ``sigma`` arguments,
      the input data array defines these, ``xdata`` by the coords on the data array,
      ``ydata`` by ``da.data``, and ``sigma`` is defined as the square root of
      the variances, if present, i.e., the standard deviations.
    - The fit function ``f`` must work with scipp objects. This provides additional
      safety over the underlying scipy function by ensuring units are consistent.
    - The initial guess in ``p0`` must be provided as a dict, mapping from fit-function
      parameter names to initial guesses.
    - The parameter bounds must also be provided as a dict, like ``p0``.
    - If the fit parameters are not dimensionless the initial guess must be
       a scipp ``Variable`` with the correct unit.
    - If the fit parameters are not dimensionless the bounds must be a variables
      with the correct unit.
    - The bounds and initial guesses may be scalars or arrays to allow the
      initial guesses or bounds to vary in different regions.
      If they are arrays they will be broadcasted to the shape of the output.
    - The returned optimal parameter values ``popt`` and the covariance matrix ``pcov``
      will have units if the initial guess has units. ``popt`` and
      ``pcov`` are ``DataGroup`` and a ``DataGroup`` of ``DataGroup`` respectively.
      They are indexed by the fit parameter names. The variances of the parameter values
      in ``popt`` are set to the corresponding diagonal value in the covariance matrix.

    Parameters
    ----------
    coords:
        The coords that act as predictor variables in the fit.
        If a mapping, the keys signify names of arguments to ``f`` and the values
        signify coordinate names in ``da.coords``. If a sequence, the names of the
        arguments to ``f`` and the names of the coords are taken to be the same.
        To use a fit coordinate not present in ``da.coords``, pass it as a Variable.
    f:
        The model function, ``f(x, y..., a, b...)``. It must take all coordinates
        listed in ``coords`` as arguments, otherwise a ``ValueError`` will be raised,
        all *other* arguments will be treated as parameters of the fit.
    da:
        The values of the data array provide the dependent data. If the data array
        stores variances then the standard deviations (square root of the variances)
        are taken into account when fitting.
    p0:
        An optional dict of initial guesses for the parameters.
        If None, then the initial values will all be dimensionless 1.
        If the fit function cannot handle initial values of 1, in particular for
        parameters that are not dimensionless, then typically a
        :py:class:``scipp.UnitError`` is raised,
        but details will depend on the function.
    bounds:
        Lower and upper bounds on parameters.
        Defaults to no bounds.
        Bounds are given as a dict of 2-tuples of (lower, upper) for each parameter
        where lower and upper are either both Variables or plain numbers.
        Parameters omitted from the ``bounds`` dict are unbounded.
    reduce_dims:
        Additional dimensions to aggregate while fitting.
        If a dimension is not in ``reduce_dims``, or in the dimensions
        of the coords used in the fit, then the values of the optimal parameters
        will depend on that dimension. One fit will be performed for every slice,
        and the data arrays in the output will have the dimension in their ``dims``.
        If a dimension is passed to ``reduce_dims`` all data in that dimension
        is instead aggregated in a single fit and the dimension will *not*
        be present in the output.
    unsafe_numpy_f:
        By default the provided fit function ``f`` is assumed to take scipp Variables
        as input and use scipp operations to produce a scipp Variable as output.
        This has the safety advantage of unit checking.
        However, in some cases it might be advantageous to implement ``f`` using Numpy
        operations for performance reasons. This is particularly the case if the
        curve fit will make many small curve fits involving relatively few data points.
        In this case the pybind overhead on scipp operations might be considerable.
        If ``unsafe_numpy_f`` is set to ``True`` then the arguments passed to ``f``
        will be Numpy arrays instead of scipp Variables and the output of ``f`` is
        expected to be a Numpy array.

    Returns
    -------
    popt:
        Optimal values for the parameters.
    pcov:
        The estimated covariance of popt.

    See Also
    --------
    scipp.scipy.optimize.curve_fit:
        Similar functionality for 1D fits.

    Examples
    --------

      A 1D example

      >>> def round(a, d):
      ...     'Helper for the doctests'
      ...     return sc.round(10**d * a) / 10**d

      >>> def func(x, a, b):
      ...     return a * sc.exp(-b * x)

      >>> rng = np.random.default_rng(1234)
      >>> x = sc.linspace(dim='x', start=0.0, stop=0.4, num=50, unit='m')
      >>> y = func(x, a=5, b=17/sc.Unit('m'))
      >>> y.values += 0.01 * rng.normal(size=50)
      >>> da = sc.DataArray(y, coords={'x': x})

      >>> from scipp import curve_fit
      >>> popt, _ = curve_fit(['x'], func, da, p0 = {'b': 1.0 / sc.Unit('m')})
      >>> round(sc.values(popt['a']), 3), round(sc.stddevs(popt['a']), 4)
      (<scipp.DataArray>
       Dimensions: Sizes[]
       Data:
                             float64  [dimensionless]  ()  4.999
       ,
       <scipp.DataArray>
       Dimensions: Sizes[]
       Data:
                             float64  [dimensionless]  ()  0.0077
       )

      A 2D example where two coordinates participate in the fit

      >>> def func(x, z, a, b):
      ...     return a * z * sc.exp(-b * x)

      >>> x = sc.linspace(dim='x', start=0.0, stop=0.4, num=50, unit='m')
      >>> z = sc.linspace(dim='z', start=0.0, stop=1, num=10)
      >>> y = func(x, z, a=5, b=17/sc.Unit('m'))
      >>> y.values += 0.01 * rng.normal(size=500).reshape(10, 50)
      >>> da = sc.DataArray(y, coords={'x': x, 'z': z})

      >>> popt, _ = curve_fit(['x', 'z'], func, da, p0 = {'b': 1.0 / sc.Unit('m')})
      >>> round(sc.values(popt['a']), 3), round(sc.stddevs(popt['a']), 3)
      (<scipp.DataArray>
       Dimensions: Sizes[]
       Data:
                             float64  [dimensionless]  ()  5.004
       ,
       <scipp.DataArray>
       Dimensions: Sizes[]
       Data:
                             float64  [dimensionless]  ()  0.004
       )

      A 2D example where only one coordinate participates in the fit and we
      map over the dimension of the other coordinate.
      Note that the value of one of the parameters is z-dependent
      and that the output has a z-dimension

      >>> def func(x, a, b):
      ...     return a * sc.exp(-b * x)

      >>> x = sc.linspace(dim='xx', start=0.0, stop=0.4, num=50, unit='m')
      >>> z = sc.linspace(dim='zz', start=0.0, stop=1, num=10)
      >>> # Note that parameter a is z-dependent.
      >>> y = func(x, a=z, b=17/sc.Unit('m'))
      >>> y.values += 0.01 * rng.normal(size=500).reshape(10, 50)
      >>> da = sc.DataArray(y, coords={'x': x, 'z': z})

      >>> popt, _ = curve_fit(
      ...    ['x'], func, da,
      ...     p0 = {'b': 1.0 / sc.Unit('m')})
      >>> # Note that approximately a = z
      >>> round(sc.values(popt['a']), 2),
      (<scipp.DataArray>
       Dimensions: Sizes[zz:10, ]
       Coordinates:
       * z      float64  [dimensionless]  (zz)  [0, 0.111111, ..., 0.888889, 1]
       Data:
                float64  [dimensionless]  (zz)  [-0.01, 0.11, ..., 0.89, 1.01]
       ,)

      Lastly, a 2D example where only one coordinate participates in the fit and
      the other coordinate is reduced.

      >>> def func(x, a, b):
      ...     return a * sc.exp(-b * x)

      >>> x = sc.linspace(dim='xx', start=0.0, stop=0.4, num=50, unit='m')
      >>> z = sc.linspace(dim='zz', start=0.0, stop=1, num=10)
      >>> y = z * 0 + func(x, a=5, b=17/sc.Unit('m'))
      >>> y.values += 0.01 * rng.normal(size=500).reshape(10, 50)
      >>> da = sc.DataArray(y, coords={'x': x, 'z': z})

      >>> popt, _ = curve_fit(
      ...    ['x'], func, da,
      ...    p0 = {'b': 1.0 / sc.Unit('m')}, reduce_dims=['zz'])
      >>> round(sc.values(popt['a']), 3), round(sc.stddevs(popt['a']), 4)
      (<scipp.DataArray>
       Dimensions: Sizes[]
       Data:
                             float64  [dimensionless]  ()  5
       ,
       <scipp.DataArray>
       Dimensions: Sizes[]
       Data:
                             float64  [dimensionless]  ()  0.0021
       )

      Note that the variance is about 10x lower in this example than in the
      first 1D example. That is because in this example 50x10 points are used
      in the fit while in the first example only 50 points were used in the fit.

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

    for c in coords:
        if c in da.coords and da.coords.is_edges(c):
            raise BinEdgeError("Cannot fit data array with bin-edge coordinate.")

    if not isinstance(coords, dict):
        if not all(isinstance(c, str) for c in coords):
            raise TypeError(
                'Expected sequence of coords to only contain values of type `str`.'
            )
        coords = {c: c for c in coords}

    # Mapping from function argument names to fit variables
    coords = {
        arg: da.coords[coord] if isinstance(coord, str) else coord
        for arg, coord in coords.items()
    }

    p0 = _make_defaults(f, coords.keys(), p0)

    f = (
        _wrap_scipp_func(f, p0)
        if not unsafe_numpy_f
        else _wrap_numpy_func(f, p0, coords.keys())
    )

    map_over = tuple(
        d
        for d in da.dims
        if d not in reduce_dims and not any(d in c.dims for c in coords.values())
    )

    dims_participating_in_fit = set(da.dims) - set(map_over)

    # Create a dataarray with only the participating coords and masks
    # and coordinate names matching the argument names of f.
    _da = DataArray(
        da.data,
        coords=coords,
        masks={
            m: da.masks[m]
            for m in da.masks
            if dims_participating_in_fit.intersection(da.masks[m].dims)
        },
    )

    out = _prepare_numpy_outputs(da, p0, map_over)

    _curve_fit(
        f,
        _da,
        p0,
        _parse_bounds(bounds, p0),
        map_over,
        unsafe_numpy_f,
        out,
        **kwargs,
    )

    return _datagroup_outputs(da, p0, map_over, *out)


__all__ = ['curve_fit']
