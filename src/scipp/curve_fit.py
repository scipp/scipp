# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
import pickle
from collections.abc import Callable, Iterable, Mapping, Sequence
from functools import partial
from inspect import getfullargspec, isfunction
from multiprocessing import Pool
from numbers import Real
from typing import Any, Sized, TypeAlias, TypeVar

import numpy as np
import numpy.typing as npt

from .core import (
    BinEdgeError,
    DataArray,
    DataGroup,
    DimensionError,
    Unit,
    Variable,
    array,
    scalar,
    stddevs,
    zeros,
)

_Shape = TypeVar('_Shape', bound=tuple[int, ...])
_DType = TypeVar('_DType', bound=np.generic)
_Float = TypeVar('_Float', np.float64, np.float32)

Bounds: TypeAlias = Mapping[
    str,
    tuple[Variable, Variable]
    | tuple[Variable, None]
    | tuple[None, Variable]
    | tuple[Real, Real]
    | tuple[Real, None]
    | tuple[None, Real]
    | tuple[None, None],
]


def _wrap_scipp_func(
    f: Callable[..., Variable], p0: Mapping[str, Variable]
) -> Callable[..., npt.NDArray[Any]]:
    p = {k: scalar(0.0, unit=v.unit) for k, v in p0.items()}

    def func(x: Mapping[str, Any], *args: Any) -> npt.NDArray[Any]:
        for k, v in zip(p, args, strict=True):
            p[k].value = v
        return f(**x, **p).values  # type: ignore[no-any-return]

    return func


def _wrap_numpy_func(
    f: Callable[..., np.ndarray[_Shape, np.dtype[_DType]]],
    param_names: Iterable[str],
    coord_names: Iterable[str],
) -> Callable[..., np.ndarray[_Shape, np.dtype[_DType]]]:
    def func(x: npt.NDArray[Any], *args: Any) -> np.ndarray[_Shape, np.dtype[_DType]]:
        # If there is only one predictor variable x might be a 1D array.
        # Make x 2D for consistency.
        if len(x.shape) == 1:
            x = x.reshape(1, -1)
        c = dict(zip(coord_names, x, strict=True))
        p = dict(zip(param_names, args, strict=True))
        return f(**c, **p)

    return func


def _get_sigma(da: DataArray) -> npt.NDArray[_Float] | None:
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


def _datagroup_outputs(
    da: DataArray,
    p0: Mapping[str, Variable],
    map_over: Iterable[str],
    pdata: npt.NDArray[Any],
    covdata: npt.NDArray[Any],
) -> tuple[DataGroup[DataArray], DataGroup[DataGroup[DataArray]]]:
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
                            unit=v0.unit * u0.unit,  # type: ignore[arg-type, operator]
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


def _prepare_numpy_outputs(
    da: DataArray, p0: Sized, map_over: Iterable[str]
) -> tuple[npt.NDArray[np.float64], npt.NDArray[np.float64]]:
    shape = [da.sizes[d] for d in map_over]
    dg = np.empty([*shape, len(p0)])
    dgcov = np.empty(shape + 2 * [len(p0)])
    return dg, dgcov


def _make_defaults(
    f: Callable[..., object],
    coords: Iterable[str],
    p0: Mapping[str, Variable | float] | None,
) -> Mapping[str, Variable]:
    spec = getfullargspec(f)
    non_default_args = (
        spec.args[: -len(spec.defaults)] if spec.defaults is not None else spec.args
    )
    if not isfunction(f) and not isinstance(f, partial):
        # f is a class with a __call__ method,
        # first argument is 'self', exclude it.
        non_default_args = non_default_args[1:]
    args = {*non_default_args, *spec.kwonlyargs} - set(spec.kwonlydefaults or ())
    if not set(coords).issubset(args):
        raise ValueError("Function must take the provided coords as arguments")
    return {
        **{a: scalar(1.0) for a in args - set(coords)},
        **{
            k: v if isinstance(v, Variable) else scalar(v)
            for k, v in (p0 or {}).items()
        },
    }


def _get_specific_bounds(
    bounds: Bounds,
    name: str,
    unit: Unit | None,
) -> tuple[Variable, Variable]:
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


def _parse_bounds(
    bounds: Bounds | None, p0: Mapping[str, Variable]
) -> dict[str, tuple[Variable, Variable]]:
    return {k: _get_specific_bounds(bounds or {}, k, v.unit) for k, v in p0.items()}


def _reshape_bounds(
    bounds: Mapping[str, tuple[Variable, Variable]],
) -> tuple[list[Any], list[Any]] | tuple[float, float]:
    left_vars, right_vars = zip(*bounds.values(), strict=True)
    left, right = [le.value for le in left_vars], [ri.value for ri in right_vars]
    if all(le == -np.inf and ri == np.inf for le, ri in zip(left, right, strict=True)):
        return -np.inf, np.inf
    return left, right


def _select_data_params_and_bounds(
    sel: tuple[str, int | slice],
    da: DataArray,
    p0: Mapping[str, Variable],
    bounds: Mapping[str, tuple[Variable, Variable]],
) -> tuple[DataArray, dict[str, Variable], dict[str, tuple[Variable, Variable]]]:
    dim, i = sel
    return (
        da[dim, i],
        {k: v[dim, i] if dim in v.dims else v for k, v in p0.items()},
        {
            k: (
                le[dim, i] if dim in le.dims else le,
                ri[dim, i] if dim in ri.dims else ri,
            )
            for k, (le, ri) in bounds.items()
        },
    )


SerializedVariable: TypeAlias = tuple[
    tuple[str, ...], npt.NDArray[Any], npt.NDArray[Any], str | None
]


def _serialize_variable(v: Variable) -> SerializedVariable:
    return v.dims, v.values, v.variances, str(v.unit) if v.unit is not None else None


SerializedMapping = tuple[tuple[str, ...], tuple[SerializedVariable, ...]]


def _serialize_mapping(v: Mapping[str, Variable]) -> SerializedMapping:
    return (tuple(v.keys()), tuple(map(_serialize_variable, v.values())))


SerializedBounds = tuple[
    tuple[str, ...], tuple[tuple[SerializedVariable, SerializedVariable], ...]
]


def _serialize_bounds(v: Mapping[str, tuple[Variable, Variable]]) -> SerializedBounds:
    return (
        tuple(v.keys()),
        tuple(
            (_serialize_variable(l), _serialize_variable(r))
            for (l, r) in v.values()  # noqa: E741
        ),
    )


SerializedDataArray = tuple[SerializedVariable, SerializedMapping, SerializedMapping]


def _serialize_data_array(da: DataArray) -> SerializedDataArray:
    return (
        _serialize_variable(da.data),
        _serialize_mapping(da.coords),
        _serialize_mapping(da.masks),
    )


def _deserialize_variable(t: SerializedVariable) -> Variable:
    return array(dims=t[0], values=t[1], variances=t[2], unit=t[3])


def _deserialize_data_array(t: SerializedDataArray) -> DataArray:
    return DataArray(
        _deserialize_variable(t[0]),
        coords=_deserialize_mapping(t[1]),
        masks=_deserialize_mapping(t[2]),
    )


def _deserialize_mapping(t: SerializedMapping) -> dict[str, Variable]:
    return dict(zip(t[0], map(_deserialize_variable, t[1]), strict=True))


def _deserialize_bounds(t: SerializedBounds) -> dict[str, tuple[Variable, Variable]]:
    return dict(
        zip(
            t[0],
            ((_deserialize_variable(l), _deserialize_variable(r)) for (l, r) in t[1]),  # noqa: E741
            strict=True,
        )
    )


def _curve_fit(
    f: Callable[..., npt.NDArray[_Float]],
    da: DataArray,
    p0: Mapping[str, Variable],
    bounds: Mapping[str, tuple[Variable, Variable]],
    map_over: Sequence[str],
    unsafe_numpy_f: bool,
    out: tuple[npt.NDArray[_Float], npt.NDArray[_Float]],
    **kwargs: object,
) -> None:
    out_values, out_cov = out

    if len(map_over) > 0:
        dim = map_over[0]
        for i in range(da.sizes[dim]):
            _curve_fit(
                f,
                *_select_data_params_and_bounds((dim, i), da, p0, bounds),
                map_over=map_over[1:],
                unsafe_numpy_f=unsafe_numpy_f,
                out=(out_values[i], out_cov[i]),
                **kwargs,  # type: ignore[arg-type]
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
    if len(fda.masks) > 0:
        _mask = zeros(dims=fda.dims, shape=fda.shape, dtype='bool')
        for mask in fda.masks.values():
            _mask |= mask
        fda = fda[~_mask]

    if not unsafe_numpy_f:
        # Making the coords into a dict improves runtime,
        # probably because of pybind overhead.
        X: dict[str, Variable] | npt.NDArray[np.float64] = dict(fda.coords)
    else:
        X = np.vstack([c.values for c in fda.coords.values()], dtype='float')

    import scipy.optimize as opt

    if len(fda) < len(out_values):
        # More parameters than data points, unable to fit, abort.
        out_values[:] = np.nan
        out_cov[:] = np.nan
        return

    try:
        popt, pcov = opt.curve_fit(
            f=f,
            xdata=X,
            ydata=fda.data.values,
            p0=[v.value for v in p0.values()],
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

    out_values[:] = popt
    out_cov[:] = pcov


def _curve_fit_chunk(
    coords: Mapping[str, Variable] | SerializedMapping,
    f: Callable[..., Variable] | Callable[..., npt.NDArray[_Float]],
    da: DataArray | SerializedDataArray,
    p0: Mapping[str, Variable] | SerializedMapping,
    bounds: Mapping[str, tuple[Variable, Variable]] | SerializedBounds,
    map_over: Sequence[str],
    unsafe_numpy_f: bool,
    kwargs: dict[str, object],
) -> tuple[npt.NDArray[np.float64], npt.NDArray[np.float64]]:
    coords = coords if isinstance(coords, Mapping) else _deserialize_mapping(coords)
    da = da if isinstance(da, DataArray) else _deserialize_data_array(da)
    p0 = p0 if isinstance(p0, Mapping) else _deserialize_mapping(p0)
    bounds = bounds if isinstance(bounds, Mapping) else _deserialize_bounds(bounds)

    f = (
        _wrap_scipp_func(f, p0)  # type: ignore[arg-type]
        if not unsafe_numpy_f
        else _wrap_numpy_func(f, p0, coords.keys())  # type: ignore[arg-type]
    )

    # Create a dataarray with only the participating coords
    _da = DataArray(da.data, coords=coords, masks=da.masks)

    out = _prepare_numpy_outputs(da, p0, map_over)

    _curve_fit(
        f=f,
        da=_da,
        p0=p0,
        bounds=bounds,
        map_over=map_over,
        unsafe_numpy_f=unsafe_numpy_f,
        out=out,
        **kwargs,
    )
    return out


def curve_fit(
    coords: Sequence[str] | Mapping[str, str | Variable],
    f: Callable[..., Variable],
    da: DataArray,
    *,
    p0: dict[str, Variable | float] | None = None,
    bounds: Bounds | None = None,
    reduce_dims: Sequence[str] = (),
    unsafe_numpy_f: bool = False,
    workers: int = 1,
    **kwargs: object,
) -> tuple[DataGroup[DataArray], DataGroup[DataGroup[DataArray]]]:
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
    workers:
        Number of worker processes to use when fitting many curves.
        Defaults to ``1``.

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
      >>> # Parameter `a` is `z`-dependent.
      >>> y = func(x, a=z, b=17/sc.Unit('m'))
      >>> y.values += 0.01 * rng.normal(size=500).reshape(10, 50)
      >>> da = sc.DataArray(y, coords={'x': x, 'z': z})

      >>> popt, _ = curve_fit(
      ...     ['x'], func, da,
      ...     p0 = {'b': 1.0 / sc.Unit('m')},
      ...     workers=1)
      >>> # Note that a = z
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

      The variance is about 10x lower in this example than in the
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
    coord_vars = {
        arg: da.coords[coord] if isinstance(coord, str) else coord
        for arg, coord in coords.items()
    }

    map_over = tuple(
        d
        for d in da.dims
        if d not in reduce_dims and not any(d in c.dims for c in coord_vars.values())
    )

    p0_vars = _make_defaults(f, coord_vars.keys(), p0)
    bounds = _parse_bounds(bounds, p0_vars)

    pardim = None
    if len(map_over) > 0:
        max_size = max((da.sizes[dim] for dim in map_over))
        max_size_dim = next((dim for dim in map_over if da.sizes[dim] == max_size))
        # Parallelize over longest dim because that is most likely
        # to give us a balanced workload over the workers.
        pardim = max_size_dim if max_size > 1 else None

    # Only parallelize if the user asked for more than one worker
    # and a suitable dimension for parallelization was found.
    if workers == 1 or pardim is None:
        par, cov = _curve_fit_chunk(
            coords=coord_vars,
            f=f,
            da=da,
            p0=p0_vars,
            bounds=bounds,
            map_over=map_over,
            unsafe_numpy_f=unsafe_numpy_f,
            kwargs=kwargs,
        )
    else:
        try:
            pickle.dumps(f)
        except (AttributeError, pickle.PicklingError) as err:
            raise ValueError(
                'The provided fit function is not pickleable and can not be used '
                'with the multiprocessing module. '
                'Either provide a function that is compatible with pickle '
                'or explicitly disable multiprocess parallelism by passing '
                'workers=1.'
            ) from err

        chunksize = (da.sizes[pardim] // workers) + 1
        args = []
        for i in range(workers):
            _da, _p0, _bounds = _select_data_params_and_bounds(
                (pardim, slice(i * chunksize, (i + 1) * chunksize)), da, p0_vars, bounds
            )
            args.append(
                (
                    _serialize_mapping(coord_vars),
                    f,
                    _serialize_data_array(_da),
                    _serialize_mapping(_p0),
                    _serialize_bounds(_bounds),
                    map_over,
                    unsafe_numpy_f,
                    kwargs,
                )
            )

        with Pool(workers) as pool:
            par, cov = zip(*pool.starmap(_curve_fit_chunk, args), strict=True)  # type: ignore[assignment]
            concat_axis = map_over.index(pardim)
            par = np.concatenate(par, axis=concat_axis)
            cov = np.concatenate(cov, axis=concat_axis)

    return _datagroup_outputs(da, p0_vars, map_over, par, cov)


__all__ = ['curve_fit']
