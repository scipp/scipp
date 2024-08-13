# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import inspect
import itertools
import uuid
from collections.abc import Sequence
from numbers import Integral
from typing import TypeVar, overload

from .._scipp import core as _cpp
from .bin_remapping import combine_bins
from .cpp_classes import BinEdgeError, CoordError, DataArray, Dataset, DType, Variable
from .data_group import DataGroup, data_group_overload
from .math import round as round_
from .shape import concat
from .variable import arange, array, epoch, linspace, scalar

_T = TypeVar('_T')


@overload
def make_histogrammed(
    x: Variable | DataArray, *, edges: Variable, erase: Sequence[str] = ()
) -> DataArray: ...


@overload
def make_histogrammed(
    x: Dataset, *, edges: Variable, erase: Sequence[str] = ()
) -> Dataset: ...


def make_histogrammed(x, *, edges, erase: Sequence[str] = ()):
    """Create dense data by histogramming data into given bins.

    If the input is binned data, then existing binning dimensions are preserved.
    Histogramming along an existing binned dimension will replace this binning.

    Usually :py:func:`scipp.hist` should be preferred.

    Parameters
    ----------
    x:
        Input data.
    edges:
        Bin edges. If these have more than one dimension, binning occurs along
        the inner dimension.

    Returns
    -------
    :
        DataArray / Dataset with values equal to the sum
        of values in each given bin.

    See Also
    --------
    scipp.hist:
        Recommended interface for histogramming data.
    scipp.bin:
        For binning data.
    """
    if isinstance(x, Variable):
        data = scalar(1.0, unit='counts').broadcast(sizes=x.sizes)
        x = DataArray(data, coords={edges.dim: x})
    elif isinstance(x, DataArray) and x.bins is not None:
        dim = edges.dims[-1]
        if dim not in x.bins.coords:
            # The second `dim` is necessary in case the coord is multi-dimensional.
            if x.coords.is_edges(dim, dim):
                raise BinEdgeError(
                    "Cannot histogram data with existing bin edges "
                    "unless event data coordinate for histogramming is available."
                )
            return make_histogrammed(x.bins.sum(), edges=edges, erase=erase)
    _check_erase_dimension_clash(erase, edges)
    # The C++ implementation uses an older heuristic histogramming a single dimension.
    # We therefore transpose and flatten the input to match this.
    hist_dim = edges.dims[-1]
    to_flatten = [dim for dim in x.dims if dim in erase]
    if hist_dim in x.dims:
        to_flatten.append(hist_dim)
    if to_flatten:
        x = _drop_coords_for_hist(x, to_flatten, keep=hist_dim)
        x = _transpose_and_flatten_for_hist(x, to_flatten, to=hist_dim)
    return _cpp.histogram(x, edges)


def _drop_coords_for_hist(x, dims: Sequence[str], keep: str) -> DataArray:
    """Drop unnecessary coords from a DataArray which would make flatten expensive."""
    to_drop = []
    for name, coord in x.coords.items():
        if name != keep and set(coord.dims) & set(dims):
            to_drop.append(name)
    return x.drop_coords(to_drop)


def _transpose_and_flatten_for_hist(x, dims: Sequence[str], to: str) -> DataArray:
    """Transpose and flatten a DataArray to prepare for histogram."""
    new_order = [dim for dim in x.dims if dim not in dims] + dims
    return x.transpose(new_order).flatten(dims=dims, to=to)


def make_binned(
    x: Variable | DataArray,
    *,
    edges: Sequence[Variable] | None = None,
    groups: Sequence[Variable] | None = None,
    erase: Sequence[str] | None = None,
) -> DataArray:
    """Create binned data by binning input along all dimensions given by edges or
    groups.

    Usually :py:func:`scipp.bin` or :py:func:`scipp.group` should be preferred,
    unless the more precise control over which dimensions should be erased is required,
    or unless grouping and binning at the same time is required.

    This does not histogram the data, each output bin will contain a "list" of
    input values.

    At least one argument of ``edges`` and ``groups`` is required.

    If the input is binned and certain bins are masked then changing the binning
    will apply the masks, i.e., masked bins are treated as empty.

    Warning
    -------

    When there is existing binning or grouping, the algorithm assumes that coordinates
    of the binned data are correct, i.e., compatible with the corresponding
    coordinate values in the individual bins. If this is not the case then the behavior
    is UNSPECIFIED. That is, the algorithm may or may not ignore the existing
    coordinates. If you encounter such as case, remove the conflicting coordinate,
    e.g., using :py:func:`scipp.DataArray.drop_coords`.

    Parameters
    ----------
    x:
        Input data.
    edges:
        Bin edges, one per dimension to bin in.
    groups:
        Keys to group input by one per dimension to group in.
    erase:
        Dimension labels to remove from output.

    Returns
    -------
    :
        Binned ``x``.

    See Also
    --------
    scipp.hist:
        For histogramming data.
    scipp.bin:
        Recommended interface for binning data.
    scipp.group:
        Recommended interface for grouping data.
    scipp.bins:
        For creating binned data based on explicitly given index ranges.
    """
    if erase is None:
        erase = []
    if groups is None:
        groups = []
    if edges is None:
        edges = []
    _check_erase_dimension_clash(erase, *edges, *groups)

    if isinstance(x, Variable) and x.bins is not None:
        x = DataArray(x)
    elif isinstance(x, Variable):
        coords = [*edges, *groups]
        if len(coords) != 1:
            raise ValueError(
                "Edges for exactly one dimension must be specified when "
                "binning or histogramming a variable."
            )
        data = scalar(1.0, unit='counts').broadcast(sizes=x.sizes).copy()
        x = DataArray(data, coords={coords[0].dim: x})
    if _can_operate_on_bins(x, edges, groups, erase):
        return combine_bins(x, edges=edges, groups=groups, dim=erase)
    # Many-to-many mapping is expensive, concat first is generally cheaper,
    # despite extra copies. If some coords are dense, perform binning in two steps,
    # since concat is not possible then (without mapping dense coords to binned coords,
    # which might bypass some other optimizations).
    if erase and x.bins is not None:
        dense_edges = [var for var in edges if var.dims[-1] not in x.bins.coords]
        dense_groups = [var for var in groups if var.dims[-1] not in x.bins.coords]
        if len(dense_edges) + len(dense_groups) == 0:
            x = x.bins.concat(erase)
            erase = ()
        elif len(dense_edges) + len(dense_groups) < len(edges) + len(groups):
            x = make_binned(x, edges=dense_edges, groups=dense_groups, erase=erase)
            edges = [var for var in edges if var.dims[-1] in x.bins.coords]
            groups = [var for var in groups if var.dims[-1] in x.bins.coords]
            erase = ()
        if x.ndim == 0:
            return (
                _cpp.bin(x.value, edges, groups, erase)
                .assign_coords(x.coords)
                .assign_masks(x.masks)
            )
    x = _prepare_multi_dim_dense(x, *edges, *groups)
    return _cpp.bin(x, edges, groups, erase)


def _prepare_multi_dim_dense(x: DataArray, *edges_or_groups: Variable) -> DataArray:
    """Prepare data for binning or grouping.

    This function is a workaround for the C++ implementation not being able to deal with
    multi-dimensional dense input data. The workaround is to flatten the data along the
    auxiliary dimensions and regroup.

    In case the ultimate operation is histogramming, this leads the desired
    higher-dimensional histogram. In case of binning or grouping, we obtain binned data
    with one additional dimension, whereas conceptually we might expect only the
    requested dimensions, with the auxiliary dimensions inside the bin content. As this
    case is likely rare and extra dimensions in bin content are barely supported in
    scipp, we consider this acceptable for now.
    """
    if x.bins is not None or x.ndim == 1:
        return x
    if any(var.ndim != 1 for var in edges_or_groups):
        raise ValueError("Cannot bin multi-dimensional dense data with ragged edges.")
    op_dims = _get_op_dims(x, *edges_or_groups)
    if len(op_dims) != 1:
        raise ValueError("Cannot bin multi-dimensional dense data along multiple dims.")
    extra = {dim for dim in x.dims if dim != next(iter(op_dims))}
    original_coords = {
        name: coord
        for name, coord in x.coords.items()
        if set(coord.dims).issubset(extra)
    }
    helper_coords = {dim: arange(dim, x.sizes[dim]) for dim in extra}
    return (
        x.assign_coords(helper_coords)
        .flatten(to=str(uuid.uuid4()))
        .group(*helper_coords.values())
        .drop_coords(tuple(extra))
        .assign_coords(original_coords)
    )


def _check_erase_dimension_clash(
    erase: Sequence[str], *edges_or_groups: Variable
) -> None:
    new_dims = set()
    for var in edges_or_groups:
        new_dims.update(var.dims)
    if set(erase) & new_dims:
        raise ValueError(
            f"Clash of dimension(s) to reduce {erase} with dimensions defined by "
            "edges or groups."
        )


def _can_operate_on_bins(x, edges, groups, erase) -> bool:
    if x.bins is None:
        return False
    dims = []
    for coord in itertools.chain(edges, groups):
        if coord.ndim != 1:
            return False
        if coord.dim in x.bins.coords:
            return False
        if coord.dim not in x.coords:
            return False
        dims += x.coords[coord.dim].dims
    return set(dims) <= set(erase)


def _require_coord(name, coord):
    if coord is None:
        raise CoordError(f"Coordinate '{name}' not found.")


def _get_coord(x, name):
    if isinstance(x, Variable):
        return x
    if isinstance(x, Dataset):
        cmin = None
        cmax = None
        for da in x.values():
            c = _get_coord(da, name)
            cmin = c.min() if cmin is None else min(cmin, c.min())
            cmax = c.max() if cmax is None else max(cmin, c.max())
        coord = concat([cmin, cmax], dim='dummy')
    else:
        event_coord = x.bins.deprecated_meta.get(name) if x.bins is not None else None
        coord = x.deprecated_meta.get(name, event_coord)
    _require_coord(name, coord)
    return coord


def _upper_bound(x: Variable) -> Variable:
    import numpy as np

    bound = x.nanmax()
    if bound.dtype in ('int32', 'int64', 'datetime64'):
        bound.value += 1
    else:
        bound.value = np.nextafter(
            bound.value, (bound + scalar(1, unit=bound.unit, dtype=bound.dtype)).value
        )
    return bound


def _parse_coords_arg(
    x: Variable | DataArray | Dataset, name: str, arg: int | Variable
) -> Variable:
    if isinstance(arg, Variable) and name in arg.dims:
        return arg
    coord = _get_coord(x, name)
    start = coord.nanmin()
    if (
        not isinstance(x, Variable)
        and (name in x.coords)
        and x.coords.is_edges(name, name)
    ):
        stop = coord.nanmax()  # existing bin-edges, do not extend
    else:
        stop = _upper_bound(coord)
    if start > stop:
        raise ValueError(
            'Empty data range, cannot automatically determine bounds. '
            'Must provide concrete bin edges.'
        )
    if isinstance(arg, Integral):
        if start.dtype == DType.datetime64:
            base = epoch(unit=start.unit)
            return base + round_(
                linspace(name, start - base, stop - base, num=arg + 1)
            ).to(dtype='int64')
        return linspace(name, start, stop, num=arg + 1).to(
            dtype=start.dtype, copy=False
        )
    step = arg.to(dtype=start.dtype, unit=start.unit)
    if step.value == 0:
        raise ValueError("Step size cannot be 0.")
    return arange(name, start, stop + step, step=step)


def _make_edges(
    x: Variable | DataArray | Dataset,
    arg_dict: dict[str, int | Variable] | None,
    kwargs: dict[str, int | Variable],
) -> dict[str, Variable]:
    if arg_dict is not None:
        kwargs = dict(**arg_dict, **kwargs)
    return {name: _parse_coords_arg(x, name, arg) for name, arg in kwargs.items()}


def _find_replaced_dims(
    x: Variable | DataArray | Dataset,
    *,
    dims: Sequence[str],
    dim: str | tuple[str, ...] | None,
) -> list[str]:
    if isinstance(x, Variable):
        replaced = x.dims
    elif dim is None:
        replaced = set()
        for name in dims:
            if name in x.coords:
                replaced.update(x.coords[name].dims)
    else:
        replaced = (dim,) if isinstance(dim, str) else dim
    return [d for d in x.dims if d in (set(replaced) - set(dims))]


@overload
def hist(
    x: Variable | DataArray,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    *,
    dim: str | tuple[str, ...] | None = None,
    **kwargs: int | Variable,
) -> DataArray: ...


@overload
def hist(
    x: Dataset,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    *,
    dim: str | tuple[str, ...] | None = None,
    **kwargs: int | Variable,
) -> Dataset: ...


@overload
def hist(
    x: DataGroup,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    *,
    dim: str | tuple[str, ...] | None = None,
    **kwargs: int | Variable,
) -> DataGroup: ...


@data_group_overload
def hist(x, arg_dict=None, /, *, dim=None, **kwargs):
    """Compute a histogram.

    Bin edges can be specified in three ways:

    1. When an integer is provided, a 'linspace' with this requested number of
       bins is created, based on the min and max of the corresponding coordinate.
    2. A scalar Scipp variable (a value with a unit) is interpreted as a target
       bin width, and an 'arange' covering the min and max of the corresponding
       coordinate is created.
    3. A custom coordinate, given as a Scipp variable with compatible unit.
       Typically this should have a single dimension matching the target dimension.

    When histogramming a dimension with an existing dimension-coord, the binning for
    the dimension is modified, i.e., the input and the output will have the same
    dimension labels.

    When histogramming by non-dimension-coords, the output will have new dimensions
    given by the names of these coordinates. These new dimensions replace the
    dimensions the input coordinates depend on.

    Parameters
    ----------
    x:
        Input data.
    arg_dict:
        Dictionary mapping dimension labels to binning parameters.
    **kwargs:
        Mapping of dimension label to corresponding binning parameters.

    Returns
    -------
    :
        Histogrammed data.

    See Also
    --------
    scipp.bin:
        Creating binned data by binning instead of summing all contributions.
    scipp.binning.make_histogrammed:
        Lower level function for histogramming that does not automatically
        replace/erase dimensions.

    Examples
    --------

    Histogram a table by one of its coord columns, specifying (1) number of bins, (2)
    bin width, or (3) actual binning:

      >>> from numpy.random import default_rng
      >>> rng = default_rng(seed=1234)
      >>> x = sc.array(dims=['row'], unit='m', values=rng.random(100))
      >>> y = sc.array(dims=['row'], unit='m', values=rng.random(100))
      >>> data = sc.ones(dims=['row'], unit='K', shape=[100])
      >>> table = sc.DataArray(data=data, coords={'x': x, 'y': y})
      >>> table.hist(x=2)
      <scipp.DataArray>
      Dimensions: Sizes[x:2, ]
      Coordinates:
      * x                         float64              [m]  (x [bin-edge])  [0.00313229, 0.497696, 0.992259]
      Data:
                                  float64              [K]  (x)  [53, 47]

      >>> table.hist(x=sc.scalar(0.2, unit='m')).sizes
      {'x': 5}

      >>> table.hist(x=sc.linspace('x', 0.2, 0.8, num=10, unit='m')).sizes
      {'x': 9}

    Histogram a table by two of its coord columns:

      >>> table.hist(x=4, y=6).sizes
      {'x': 4, 'y': 6}

    Histogram binned data, using existing bins:

      >>> binned = table.bin(x=10)
      >>> binned.hist().sizes
      {'x': 10}

    Histogram binned data, using new bins along existing dimension:

      >>> binned = table.bin(x=10)
      >>> binned.hist(x=20).sizes
      {'x': 20}

    Histogram binned data along an additional dimension:

      >>> binned = table.bin(x=10)
      >>> binned.hist(y=5).sizes
      {'x': 10, 'y': 5}
    """  # noqa: E501
    edges = _make_edges(x, arg_dict, kwargs)
    erase = _find_replaced_dims(x, dims=edges, dim=dim)
    if isinstance(x, Variable) and len(edges) != 1:
        raise ValueError(
            "Edges for exactly one dimension must be specified when "
            "binning or histogramming a variable."
        )
    if len(edges) == 0:
        if x.bins is None:
            raise TypeError("Data is not binned so bin edges must be provided.")
        return x.bins.sum()
    if len(edges) == 1:
        # TODO Note that this may swap dims, is that ok?
        out = make_histogrammed(x, edges=next(iter(edges.values())), erase=erase)
    else:
        # Drop coords that would disappear by histogramming, to avoid costly handling
        # in intermediate binning step.
        if isinstance(x, DataArray):
            x = _drop_unused_coords(x, edges)
        elif isinstance(x, Dataset):
            x = Dataset({k: _drop_unused_coords(v, edges) for k, v in x.items()})
        edges = list(edges.values())
        # If histogramming by the final edges needs to use a non-event coord then we
        # must not erase that dim, since it removes the coord required for histogramming
        remaining_erase = set(erase)
        if isinstance(x, DataArray) and x.bins is not None:
            hist_dim = edges[-1].dims[-1]
            if hist_dim not in x.bins.coords:
                erase = [e for e in erase if e not in x.coords[hist_dim].dims]
        remaining_erase -= set(erase)
        out = make_histogrammed(
            make_binned(x, edges=edges[:-1], erase=erase),
            edges=edges[-1],
            erase=remaining_erase,
        )
    return out


def _drop_unused_coords(x: DataArray, edges: dict[str, Variable]) -> DataArray:
    da = x if x.bins is None else x.bins
    op_dims = _get_op_dims(da, *edges.values())
    drop = list(
        {coord for coord in da.coords if set(da.coords[coord].dims) & op_dims}
        - set(edges)
    )
    if x.bins is None:
        x = x.drop_coords(drop)
    else:
        content = x.bins.constituents
        content['data'] = content['data'].drop_coords(drop)
        x = x.copy(deep=False)
        x.data = _cpp._bins_no_validate(**content)
    return x


def _get_op_dims(x: DataArray, *edges_or_groups: Variable) -> set[str]:
    edge_dims = {edge.dims[-1] for edge in edges_or_groups}
    coords = [x.coords[dim] for dim in edge_dims if dim in x.coords]
    return {coord.dims[-1] for coord in coords if coord.ndim > 0}


@overload
def nanhist(
    x: Variable | DataArray,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    *,
    dim: str | tuple[str, ...] | None = None,
    **kwargs: int | Variable,
) -> DataArray: ...


@overload
def nanhist(
    x: DataGroup,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    *,
    dim: str | tuple[str, ...] | None = None,
    **kwargs: int | Variable,
) -> DataGroup: ...


@data_group_overload
def nanhist(
    x: DataArray,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    *,
    dim: str | tuple[str, ...] | None = None,
    **kwargs: int | Variable,
) -> DataArray:
    """Compute a histogram, skipping NaN values.

    Like :py:func:`scipp.hist`, but NaN values are skipped. See there for details and
    examples.

    Parameters
    ----------
    x:
        Input data.
    arg_dict:
        Dictionary mapping dimension labels to binning parameters.
    **kwargs:
        Mapping of dimension label to corresponding binning parameters.

    Returns
    -------
    :
        Histogrammed data.
    """
    edges = _make_edges(x, arg_dict, kwargs)
    if len(edges) > 0:
        x = x.bin(edges, dim=dim)
    if x.bins is None:
        raise TypeError("Data is not binned so bin edges must be provided.")
    return x.bins.nansum()


@overload
def bin(
    x: Variable | DataArray,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    *,
    dim: str | tuple[str, ...] | None = None,
    **kwargs: int | Variable,
) -> DataArray: ...


@overload
def bin(
    x: DataGroup,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    *,
    dim: str | tuple[str, ...] | None = None,
    **kwargs: int | Variable,
) -> DataGroup: ...


@data_group_overload
def bin(
    x: Variable | DataArray,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    *,
    dim: str | tuple[str, ...] | None = None,
    **kwargs: int | Variable,
) -> DataArray:
    """Create binned data by binning input along all dimensions given by edges.

    Bin edges can be specified in three ways:

    1. When an integer is provided, a 'linspace' with this requested number of
       bins is created, based on the min and max of the corresponding coordinate.
    2. A scalar Scipp variable (a value with a unit) is interpreted as a target
       bin width, and an 'arange' covering the min and max of the corresponding
       coordinate is created.
    3. A custom coordinate, given as a Scipp variable with compatible unit.
       Typically, this should have a single dimension matching the target dimension.

    When binning a dimension with an existing dimension-coord, the binning for
    the dimension is modified, i.e., the input and the output will have the same
    dimension labels.

    When binning by non-dimension-coords, the output will have new dimensions
    given by the names of these coordinates. These new dimensions replace the
    dimensions the input coordinates depend on.

    Warning
    -------

    When there is existing binning or grouping, the algorithm assumes that coordinates
    of the binned data are correct, i.e., compatible with the corresponding
    coordinate values in the individual bins. If this is not the case then the behavior
    if UNSPECIFIED. That is, the algorithm may or may not ignore the existing
    coordinates. If you encounter such as case, remove the conflicting coordinate,
    e.g., using :py:func:`scipp.DataArray.drop_coords`.

    Parameters
    ----------
    x:
        Input data.
    arg_dict:
        Dictionary mapping dimension labels to binning parameters.
    **kwargs:
        Mapping of dimension label to corresponding binning parameters.

    Returns
    -------
    :
        Binned data.

    See Also
    --------
    scipp.hist:
        For histogramming data.
    scipp.group:
        Creating binned data by grouping, instead of binning based on edges.
    scipp.binning.make_binned:
        Lower level function that can bin and group, and does not automatically
        replace/erase dimensions.

    Examples
    --------

    Bin a table by one of its coord columns, specifying (1) number of bins, (2)
    bin width, or (3) actual binning:

      >>> from numpy.random import default_rng
      >>> rng = default_rng(seed=1234)
      >>> x = sc.array(dims=['row'], unit='m', values=rng.random(100))
      >>> y = sc.array(dims=['row'], unit='m', values=rng.random(100))
      >>> data = sc.ones(dims=['row'], unit='K', shape=[100])
      >>> table = sc.DataArray(data=data, coords={'x': x, 'y': y})
      >>> table.bin(x=2).sizes
      {'x': 2}

      >>> table.bin(x=sc.scalar(0.2, unit='m')).sizes
      {'x': 5}

      >>> table.bin(x=sc.linspace('x', 0.2, 0.8, num=10, unit='m')).sizes
      {'x': 9}

    Bin a table by two of its coord columns:

      >>> table.bin(x=4, y=6).sizes
      {'x': 4, 'y': 6}

    Bin binned data, using new bins along existing dimension:

      >>> binned = table.bin(x=10)
      >>> binned.bin(x=20).sizes
      {'x': 20}

    Bin binned data along an additional dimension:

      >>> binned = table.bin(x=10)
      >>> binned.bin(y=5).sizes
      {'x': 10, 'y': 5}
    """
    edges = _make_edges(x, arg_dict, kwargs)
    erase = _find_replaced_dims(x, dims=edges, dim=dim)
    return make_binned(x, edges=list(edges.values()), erase=erase)


@overload
def rebin(
    x: DataArray,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    **kwargs: int | Variable,
) -> DataArray: ...


@overload
def rebin(
    x: Dataset,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    **kwargs: int | Variable,
) -> Dataset: ...


@overload
def rebin(
    x: DataGroup,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    **kwargs: int | Variable,
) -> DataGroup: ...


@data_group_overload
def rebin(
    x: _T,
    arg_dict: dict[str, int | Variable] | None = None,
    /,
    **kwargs: int | Variable,
) -> _T:
    """Rebin a data array or dataset.

    The coordinate of the input for the dimension to be rebinned must contain bin edges,
    i.e., the data must be histogrammed.

    If the input has masks that contain the dimension being rebinned then those
    masks are applied to the data before rebinning. That is, masked values are treated
    as zero.

    Parameters
    ----------
    x:
        Data to rebin.
    arg_dict:
        Dictionary mapping dimension labels to binning parameters.
    **kwargs:
        Mapping of dimension label to corresponding binning parameters.

    Returns
    -------
    :
        Data rebinned according to the new bin edges.

    See Also
    --------
    scipp.bin:
        For changing the binning of binned (as opposed to dense, histogrammed) data.
    scipp.hist:
        For histogramming data.

    Examples
    --------

    Rebin a data array along one of its dimensions, specifying (1) number of bins, (2)
    bin width, or (3) actual binning:

      >>> from numpy.random import default_rng
      >>> rng = default_rng(seed=1234)
      >>> x = sc.array(dims=['row'], unit='m', values=rng.random(100))
      >>> y = sc.array(dims=['row'], unit='m', values=rng.random(100))
      >>> data = sc.ones(dims=['row'], unit='K', shape=[100])
      >>> table = sc.DataArray(data=data, coords={'x': x, 'y': y})
      >>> da = table.hist(x=100, y=100)
      >>> da.rebin(x=2).sizes
      {'x': 2, 'y': 100}

      >>> da.rebin(x=sc.scalar(0.2, unit='m')).sizes
      {'x': 5, 'y': 100}

      >>> da.rebin(x=sc.linspace('x', 0.2, 0.8, num=10, unit='m')).sizes
      {'x': 9, 'y': 100}

    Rebin a data array along two of its dimensions:

      >>> da = table.hist(x=100, y=100)
      >>> da.rebin(x=4, y=6).sizes
      {'x': 4, 'y': 6}
    """
    edges = _make_edges(x, arg_dict, kwargs)
    out = x
    for dim, edge in edges.items():
        out = _cpp.rebin(out, dim, edge)
    return out


def _make_groups(x, arg):
    import numpy as np

    if isinstance(arg, Variable):
        return arg
    coord = x.bins.coords.get(arg) if x.bins is not None else None
    if coord is None:
        coord = x.coords.get(arg)
    _require_coord(arg, coord)
    if coord.bins is not None:
        coord = coord.copy().bins.constituents['data']

    if 0 in coord.shape:
        unique = coord.values[0:0]
    # We are currently using np.unique to find all unique groups. This can be very slow
    # for large inputs. In many cases groups are in a bounded range of integers, and we
    # can sometimes bypass a full call to np.unique by checking a sub-range first
    elif coord.dtype in (DType.int32, DType.int64):
        min_ = coord.min().value
        max_ = coord.max().value
        values = coord.values
        unique = values[0:0]
        for pivot in [1000, 100, 10, 1]:
            if len(unique) == max_ - min_ + 1:
                break
            unique = np.unique(values[: len(values) // pivot])
    else:
        unique = np.unique(coord.values)
    return array(dims=[arg], values=unique, unit=coord.unit)


@overload
def group(
    x: DataArray,
    /,
    *args: str | Variable,
    dim: str | tuple[str, ...] | None = None,
) -> DataArray: ...


@overload
def group(
    x: DataGroup,
    /,
    *args: str | Variable,
    dim: str | tuple[str, ...] | None = None,
) -> DataGroup: ...


@data_group_overload
def group(
    x: DataArray,
    /,
    *args: str | Variable,
    dim: str | tuple[str, ...] | None = None,
) -> DataArray:
    """Create binned data by grouping input by one or more coordinates.

    Grouping can be specified in two ways: (1) When a string is provided the unique
    values of the corresponding coordinate are used as groups. (2) When a Scipp variable
    is provided then the variable's values are used as groups.

    Note that option (1) may be very slow if the input is very large.

    When grouping a dimension with an existing dimension-coord, the binning for
    the dimension is modified, i.e., the input and the output will have the same
    dimension labels.

    When grouping by non-dimension-coords, the output will have new dimensions
    given by the names of these coordinates. These new dimensions replace the
    dimensions the input coordinates depend on.

    Warning
    -------

    When there is existing binning or grouping, the algorithm assumes that coordinates
    of the binned data are correct, i.e., compatible with the corresponding
    coordinate values in the individual bins. If this is not the case then the behavior
    if UNSPECIFIED. That is, the algorithm may or may not ignore the existing
    coordinates. If you encounter such as case, remove the conflicting coordinate,
    e.g., using :py:func:`scipp.DataArray.drop_coords`.

    Parameters
    ----------
    x:
        Input data.
    *args:
        Dimension labels or grouping variables.

    Returns
    -------
    :
        Binned data.

    See Also
    --------
    scipp.bin:
        Creating binned data by binning based on edges, instead of grouping.
    scipp.binning.make_binned:
        Lower level function that can bin and group, and does not automatically
        replace/erase dimensions.

    Examples
    --------

    Group a table by one of its coord columns, specifying (1) a coord name or (2)
    an actual grouping:

      >>> from numpy.random import default_rng
      >>> rng = default_rng(seed=1234)
      >>> x = sc.array(dims=['row'], unit='m', values=rng.random(100))
      >>> y = sc.array(dims=['row'], unit='m', values=rng.random(100))
      >>> data = sc.ones(dims=['row'], unit='K', shape=[100])
      >>> table = sc.DataArray(data=data, coords={'x': x, 'y': y})
      >>> table.coords['label'] = (table.coords['x'] * 10).to(dtype='int64')
      >>> table.group('label').sizes
      {'label': 10}

      >>> groups = sc.array(dims=['label'], values=[1, 3, 5], unit='m')
      >>> table.group(groups).sizes
      {'label': 3}

    Group a table by two of its coord columns:

      >>> table.coords['a'] = (table.coords['x'] * 10).to(dtype='int64')
      >>> table.coords['b'] = (table.coords['y'] * 10).to(dtype='int64')
      >>> table.group('a', 'b').sizes
      {'a': 10, 'b': 10}

      >>> groups = sc.array(dims=['a'], values=[1, 3, 5], unit='m')
      >>> table.group(groups, 'b').sizes
      {'a': 3, 'b': 10}

    Group binned data along an additional dimension:

      >>> table.coords['a'] = (table.coords['y'] * 10).to(dtype='int64')
      >>> binned = table.bin(x=10)
      >>> binned.group('a').sizes
      {'x': 10, 'a': 10}
    """
    groups = [_make_groups(x, name) for name in args]
    erase = _find_replaced_dims(x, dims=[g.dim for g in groups], dim=dim)
    return make_binned(x, groups=groups, erase=erase)


VARIABLE_BINNING_SIGNATURE = inspect.Signature(
    parameters=[
        inspect.Parameter(name='self', kind=inspect.Parameter.POSITIONAL_ONLY),
        inspect.Parameter(
            name='arg_dict',
            kind=inspect.Parameter.POSITIONAL_ONLY,
            annotation=dict[str, int | Variable] | None,
            default=None,
        ),
        inspect.Parameter(
            name='kwargs', kind=inspect.Parameter.VAR_KEYWORD, annotation=int | Variable
        ),
    ],
    return_annotation=DataArray,
)
