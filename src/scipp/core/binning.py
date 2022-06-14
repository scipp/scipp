# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import warnings
from typing import Dict, List, Optional, Union, Sequence

from .._scipp import core as _cpp
from .variable import array, Variable, linspace, arange
from .operations import _rebin


def make_histogrammed(x: Union[_cpp.DataArray, _cpp.Dataset], *,
                      bins: _cpp.Variable) -> Union[_cpp.DataArray, _cpp.Dataset]:
    """Create dense data by histogramming data along all dimension given by
    edges.

    Returns
    -------
    :
        DataArray / Dataset with values equal to the sum
        of values in each given bin.

    See Also
    --------
    scipp.bin:
        For binning data.
    """
    return _cpp.histogram(x, bins)


def make_binned(x: _cpp.DataArray,
                *,
                edges: Optional[Sequence[_cpp.Variable]] = None,
                groups: Optional[Sequence[_cpp.Variable]] = None,
                erase: Optional[Sequence[_cpp.Variable]] = None) -> _cpp.DataArray:
    """Create binned data by binning input along all dimensions given by edges.

    This does not histogram the data, each output bin will contain a "list" of
    input values.

    At least one argument of ``edges`` and ``groups`` is required.

    If the input is binned and certain bins are masked then changing the binning
    will apply the masks, i.e., masked bins are treated as empty.

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
    scipp.histogram:
        For histogramming data.
    scipp.bins:
        For creating binned data based on explicitly given index ranges.
    """
    if erase is None:
        erase = []
    if groups is None:
        groups = []
    if edges is None:
        edges = []
    return _cpp.bin(x, edges, groups, erase)


def _require_coord(name, coord):
    if coord is None:
        raise _cpp.CoordError(f"Coordinate '{name}' not found.")


def _get_coord(x, name):
    event_coord = x.bins.meta.get(name) if x.bins is not None else None
    coord = x.meta.get(name, event_coord)
    _require_coord(name, coord)
    return coord


def _upper_bound(x):
    import numpy as np
    bound = x.max()
    if bound.dtype in ('int32', 'int64'):
        bound.value += 1
    else:
        bound.value = np.nextafter(bound.value, np.inf)
    return bound


def _parse_coords_arg(x, name, arg):
    if isinstance(arg, Variable) and name in arg.dims:
        return arg
    coord = _get_coord(x, name)
    if isinstance(arg, int):
        start = coord.min()
        return linspace(name, start, _upper_bound(coord),
                        num=arg + 1).to(dtype=start.dtype, copy=False)
    start = coord.min()
    step = arg.to(dtype=start.dtype, unit=start.unit)
    stop = _upper_bound(coord) + step
    # See #2639, need explicit dtype currently
    return arange(name, start, stop, step=step, dtype=start.dtype)


def _make_edges(x: Union[_cpp.DataArray,
                         _cpp.Dataset], arg_dict: Dict[str, Union[int, Variable]],
                kwargs: Dict[str, Union[int, Variable]]) -> List[Variable]:
    if arg_dict is not None:
        kwargs = dict(**arg_dict, **kwargs)
    return {name: _parse_coords_arg(x, name, arg) for name, arg in kwargs.items()}


def _find_replaced_dims(x, dims):
    if x.bins is None:
        return []
    erase = set()
    for dim in dims:
        if (coord := x.meta.get(dim)) is not None:
            if dim not in coord.dims:
                erase = erase.union(coord.dims)
    return [dim for dim in erase if dim not in dims]


def hist(x: Union[_cpp.DataArray, _cpp.Dataset],
         arg_dict: Optional[Dict[str, Union[int, Variable]]] = None,
         /,
         **kwargs: Union[int, Variable]) -> Union[_cpp.DataArray, _cpp.Dataset]:
    """Compute a histogram.

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
        Mapping of dimension label to correspoding binning parameters.

    Returns
    -------
    :
        Histogrammed data.

    Examples
    --------

    Histogram a table by one of its coord columns, specifying (1) number of bins, (2)
    bin width, or (3) actual binning:

      >>> table = sc.data.table_xyz(100)
      >>> da = table.hist(x=2)
      >>> da = table.hist(x=sc.scalar(0.2, unit='m'))
      >>> da = table.hist(x=sc.linspace('x', 0.2, 0.8, num=10, unit='m'))

    Histogram a table by two of its coord columns:

      >>> table = sc.data.table_xyz(100)
      >>> da = table.hist(x=4, y=6)

    Histogram binned data, using existing bins:

      >>> binned = sc.data.binned_x(nevent=100, nbin=10)
      >>> da = binned.hist()

    Histogram binned data, using new bins along existing dimension:

      >>> binned = sc.data.binned_x(nevent=100, nbin=10)
      >>> da = binned.hist(x=20)

    Histogram binned data along an additional dimension:

      >>> binned = sc.data.binned_x(nevent=100, nbin=10)
      >>> da = binned.hist(y=5)
    """
    edges = _make_edges(x, arg_dict, kwargs)
    erase = _find_replaced_dims(x, edges)
    if len(edges) == 0:
        return x.bins.sum()
    if len(edges) == 1:
        # TODO Note that this may swap dims, is that ok?
        out = make_histogrammed(x, bins=list(edges.values())[0])
    else:
        edges = list(edges.values())
        out = make_histogrammed(make_binned(x, edges=edges[:-1], erase=erase),
                                bins=edges[-1])
    for dim in erase:
        if dim in out.dims:
            out = out.sum(dim)
    return out


def nanhist(x: Union[_cpp.DataArray, _cpp.Dataset],
            arg_dict: Optional[Dict[str, Union[int, Variable]]] = None,
            /,
            **kwargs: Union[int, Variable]) -> Union[_cpp.DataArray, _cpp.Dataset]:
    edges = _make_edges(x, arg_dict, kwargs)
    if len(edges) > 0:
        x = x.bin(edges)
    return x.bins.nansum()


def bin(x: Union[_cpp.DataArray, _cpp.Dataset],
        arg_dict: Dict[str, Union[int, Variable]] = None,
        /,
        **kwargs: Union[int, Variable]) -> Union[_cpp.DataArray, _cpp.Dataset]:
    if arg_dict is None:
        for name, item in kwargs.items():
            if name in ('edges', 'groups', 'erase') and isinstance(item, list):
                warnings.warn(
                    "The 'edges', 'groups', and 'erase' keyword arguments "
                    "are deprecated. Use, e.g., 'sc.bin(da, x=x_edges)' or "
                    "'sc.group(da, groups)'. See the documentation for details.",
                    DeprecationWarning)
                return make_binned(x, **kwargs)
    edges = _make_edges(x, arg_dict, kwargs)
    erase = _find_replaced_dims(x, edges)
    return make_binned(x, edges=list(edges.values()), erase=erase)


def rebin(x: Union[_cpp.DataArray, _cpp.Dataset],
          arg_dict: Dict[str, Union[int, Variable]] = None,
          deprecated=None,
          /,
          **kwargs: Union[int, Variable]) -> Union[_cpp.DataArray, _cpp.Dataset]:
    if isinstance(arg_dict, str):
        if deprecated is not None or 'bins' in kwargs:
            warnings.warn(
                "The 'bins' keyword argument and positional syntax for setting bin "
                "edges is deprecated. Use, e.g., 'sc.rebin(da, x=x_edges)'. See the "
                "documentation for details.", DeprecationWarning)
            bins = {'bins': deprecated, **kwargs}
            return _rebin(x, arg_dict, **bins)
    edges = _make_edges(x, arg_dict, kwargs)
    out = x
    for dim, edge in edges.items():
        out = _rebin(out, dim, edge)
    return out


def _make_groups(x, arg):
    import numpy as np
    if isinstance(arg, Variable):
        return arg
    coord = x.bins.meta.get(arg) if x.bins is not None else None
    if coord is None:
        coord = x.meta.get(arg)
    _require_coord(arg, coord)
    if coord.bins is not None:
        coord = coord.copy().bins.constituents['data']
    # TODO Very inefficient np.unique
    return array(dims=[arg], values=np.unique(coord.values), unit=coord.unit)


def group(x: Union[_cpp.DataArray, _cpp.Dataset], /,
          *args: Union[str, Variable]) -> Union[_cpp.DataArray, _cpp.Dataset]:
    groups = [_make_groups(x, name) for name in args]
    erase = _find_replaced_dims(x, [g.dim for g in groups])
    return make_binned(x, groups=groups, erase=erase)


def histogram(x: Union[_cpp.DataArray, _cpp.Dataset], *,
              bins: _cpp.Variable) -> Union[_cpp.DataArray, _cpp.Dataset]:
    """Deprecated. See :py:func:`scipp.hist`."""
    warnings.warn("'histogram' is deprecated. Use 'hist' instead.", DeprecationWarning)
    return make_histogrammed(x, bins=bins)
