# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Dict, Optional, Sequence, Union

from .._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from ..typing import VariableLike, MetaDataMap
from .domains import merge_equal_adjacent
from .operations import islinspace
from .variable import Variable, linspace, arange


class Lookup:

    def __init__(self, func: _cpp.DataArray, dim: str):
        if func.ndim == 1 and func.dtype in [
                _cpp.DType.bool, _cpp.DType.int32, _cpp.DType.int64
        ] and not islinspace(func.coords[dim], dim).value:
            # Significant speedup if `func` is large but mostly constant.
            func = merge_equal_adjacent(func)
        self.func = func
        self.dim = dim

    def __getitem__(self, var):
        return _cpp.buckets.map(self.func, var, self.dim)


def lookup(func: _cpp.DataArray, dim: str):
    """Create a "lookup table" from a histogram (data array with bin-edge coord).

    The lookup table can be used to map, e.g., time-stamps to corresponding values
    given by a time-series log.

    Parameters
    ----------
    func:
        Histogram defining the lookup table.
    dim:
        Dimension along which the lookup occurs.

    Examples
    --------

      >>> x = sc.linspace(dim='x', start=0.0, stop=1.0, num=4)
      >>> vals = sc.array(dims=['x'], values=[3, 2, 1])
      >>> hist = sc.DataArray(data=vals, coords={'x': x})
      >>> sc.lookup(hist, 'x')[sc.array(dims=['event'], values=[0.1,0.4,0.1,0.6,0.9])]
      <scipp.Variable> (event: 5)      int64  [dimensionless]  [3, 2, ..., 2, 1]
    """
    return Lookup(func, dim)


class Bins:
    """Proxy for access to bin contents and operations on bins of a variable.

    This class is returned from the `bins` property of variables and should
    generally not be created directly.
    """

    def __init__(self, obj):
        self._obj = obj

    def _data(self):
        try:
            return self._obj.data
        except AttributeError:
            return self._obj

    def __mul__(self, lut: lookup):
        copy = self._obj.copy()
        _cpp.buckets.scale(copy, lut.func, lut.dim)
        return copy

    def __truediv__(self, lut: lookup):
        copy = self._obj.copy()
        _cpp.buckets.scale(copy, _cpp.reciprocal(lut.func), lut.dim)
        return copy

    def __imul__(self, lut: lookup):
        _cpp.buckets.scale(self._obj, lut.func, lut.dim)
        return self

    def __itruediv__(self, lut: lookup):
        _cpp.buckets.scale(self._obj, _cpp.reciprocal(lut.func), lut.dim)
        return self

    @property
    def coords(self) -> MetaDataMap:
        """Coords of the bins"""
        return _cpp._bins_view(self._data()).coords

    @property
    def meta(self) -> MetaDataMap:
        """Coords and attrs of the bins"""
        return _cpp._bins_view(self._data()).meta

    @property
    def attrs(self) -> MetaDataMap:
        """Coords of the bins"""
        return _cpp._bins_view(self._data()).attrs

    @property
    def masks(self) -> MetaDataMap:
        """Masks of the bins"""
        return _cpp._bins_view(self._data()).masks

    @property
    def data(self) -> _cpp.Variable:
        """Data of the bins"""
        return _cpp._bins_view(self._data()).data

    @data.setter
    def data(self, data: _cpp.Variable):
        """Set data of the bins"""
        _cpp._bins_view(self._data()).data = data

    @property
    def unit(self) -> _cpp.Unit:
        """Unit of the bin elements"""
        return self.constituents['data'].unit

    @unit.setter
    def unit(self, unit: Union[_cpp.Unit, str]):
        """Set unit of the bin elements"""
        self.constituents['data'].unit = unit

    @property
    def constituents(self) -> Dict[str, Union[str, _cpp.Variable, _cpp.DataArray]]:
        """Constituents of binned data, as supported by :py:func:`sc.bins`."""
        return _call_cpp_func(_cpp.bins_constituents, self._data())

    def sum(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Sum of events in each bin.

        Returns
        -------
        :
            The sum of each of the input bins.

        See Also
        --------
        scipp.sum:
            For summing non-bin data or summing bins.
        """
        return _call_cpp_func(_cpp.bins_sum, self._obj)

    def nansum(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Sum of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The sum of each of the input bins without NaN's.

        See Also
        --------
        scipp.nansum:
            For summing non-bin data or summing bins.
        """
        return _call_cpp_func(_cpp.bins_nansum, self._obj)

    def mean(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Arithmetic mean of events in each bin.

        Returns
        -------
        :
            The mean of each of the input bins.

        See Also
        --------
        scipp.mean:
            For calculating the mean of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_mean, self._obj)

    def nanmean(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Arithmetic mean of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The mean of each of the input bins without NaN's.

        See Also
        --------
        scipp.nanmean:
            For calculating the mean of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_nanmean, self._obj)

    def max(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Maximum of events in each bin.

        Returns
        -------
        :
            The maximum of each of the input bins.

        See Also
        --------
        scipp.max:
            For calculating the maximum of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_max, self._obj)

    def nanmax(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Maximum of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The maximum of each of the input bins without NaN's.

        See Also
        --------
        scipp.nanmax:
            For calculating the maximum of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_nanmax, self._obj)

    def min(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Minimum of events in each bin.

        Returns
        -------
        :
            The minimum of each of the input bins.

        See Also
        --------
        scipp.min:
            For calculating the minimum of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_min, self._obj)

    def nanmin(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Minimum of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The minimum of each of the input bins without NaN's.

        See Also
        --------
        scipp.nanmin:
            For calculating the minimum of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_nanmin, self._obj)

    def all(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Logical AND of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The AND of each of the input bins without NaN's.

        See Also
        --------
        scipp.all:
            For performing an AND of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_all, self._obj)

    def any(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Logical OR of events in each bin ignoring NaN's.

        Returns
        -------
        :
            The OR of each of the input bins without NaN's.

        See Also
        --------
        scipp.all:
            For performing an OR of non-bin data or across bins.
        """
        return _call_cpp_func(_cpp.bins_any, self._obj)

    def size(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Number of events or elements in a bin.

        Returns
        -------
        :
            The number of elements in each of the input bins.
        """
        return _call_cpp_func(_cpp.bin_sizes, self._obj)

    def concat(self, dim: Optional[str] = None) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Concatenate bins element-wise by concatenating bin contents along
        their internal bin dimension.

        This is a reduction operation similar to :py:func:`scipp.sum` but operates on
        binned data. Elements (bins) are concatenated along their internal dimension.

        Parameters
        ----------
        dim:
            Reduction dimension.

        Returns
        -------
        :
            All bins along `dim` concatenated into a single bin.
        """
        if dim is not None:
            return _call_cpp_func(_cpp.buckets.concatenate, self._obj, dim)
        raise RuntimeError("Reduction along all dims not supported yet.")

    def concatenate(
            self,
            other: Union[_cpp.Variable, _cpp.DataArray],
            *,
            out: Optional[_cpp.DataArray] = None
    ) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Concatenate bins element-wise by concatenating bin contents along
        their internal bin dimension.

        The bins to concatenate are obtained element-wise from `self` and `other`.

        Parameters
        ----------
        other:
            Other input containing bins.
        out:
            Optional output buffer.

        Returns
        -------
        :
            The bins of the two inputs merged.

        Raises
        ------
        scipp.DTypeError
            If `other` is not binned data.
        """
        if out is None:
            return _call_cpp_func(_cpp.buckets.concatenate, self._obj, other)
        else:
            if self._obj is out:
                _call_cpp_func(_cpp.buckets.append, self._obj, other)
            else:
                out = _call_cpp_func(_cpp.buckets.concatenate, self._obj, other)
            return out


class GroupbyBins:
    """Proxy for operations on bins of a groupby object."""

    def __init__(self, obj):
        self._obj = obj

    def concat(self, dim):
        return self._obj.concat(dim)


def _bins(obj):
    """
    Returns helper :py:class:`scipp.Bins` allowing bin-wise operations
    to be performed or `None` if not binned data.
    """
    if _cpp.is_bins(obj):
        return Bins(obj)
    else:
        return None


def _set_bins(obj, bins: Bins):
    # Should only be used by __iadd__ and friends
    assert obj is bins._obj


def _groupby_bins(obj):
    return GroupbyBins(obj)


def _get_coord(x, name):
    event_coord = x.bins.meta.get(name) if x.bins is not None else None
    return x.meta.get(name, event_coord)


def _upper_bound(x):
    import numpy as np
    bound = x.max()
    bound.value = np.nextafter(bound.value, np.inf)
    return bound


def _parse_coords_arg(x, name, arg):
    coord = _get_coord(x, name)
    if isinstance(arg, int):
        coord = linspace(name, coord.min(), _upper_bound(coord), num=arg + 1)
    elif isinstance(arg, Variable):
        if arg.ndim == 0:
            start = coord.min()
            step = arg.to(dtype=start.dtype, unit=start.unit)
            stop = _upper_bound(coord) + step
            coord = arange(name, start, stop, step=step)
        else:
            coord = arg
    return coord


def histogram(x: Union[_cpp.DataArray, _cpp.Dataset],
              edges: Dict[str, Union[int, Variable]] = None,
              /,
              **kwargs) -> Union[_cpp.DataArray, _cpp.Dataset]:
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
    if edges is not None:
        kwargs = dict(**edges, **kwargs)
    if len(kwargs) != 1:
        raise ValueError("Currently only 1-D histogramming is supported.")
    name, arg = next(iter(kwargs.items()))
    return _cpp.histogram(x, bins=_parse_coords_arg(x, name, arg))


def bin(x: _cpp.DataArray,
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
    return _call_cpp_func(_cpp.bin, x, edges, groups, erase)


def bins(*,
         data: VariableLike,
         dim: str,
         begin: Optional[_cpp.Variable] = None,
         end: Optional[_cpp.Variable] = None) -> _cpp.Variable:
    """Create a binned variable from bin indices.

    The elements of the returned variable are "bins", defined as views into
    ``data``. The returned variable keeps and manages a copy of ``data``
    internally.

    The variables ``begin`` and ``end`` must have the same dims and shape and
    ``dtype=sc.DType.int64``. The output dims and shape are given by ``begin``.
    If only ``begin`` is given, each bucket is a slice containing a non-range
    slice of ``data`` at the given indices. If neither ``begin`` nor ``end``
    are given, the output has ``dims=[dim]`` and contains all non-range slices
    along that dimension.

    Parameters
    ----------
    begin:
        Optional begin indices of bins, used for slicing ``data``.
        If not provided each row of ``data`` is mapped to a different bin.
    end:
        Optional end indices of bins, used for slicing ``data``. If not
        provided this is assumed to be ``begin + 1``.
    dim:
        Dimension of ``data`` that will be sliced to obtain data for
        any given bin.
    data:
        A variable, data array, or dataset containing combined data of all bins.

    Returns
    -------
    :
        Variable containing data in bins.

    See Also
    --------
    scipp.bin:
        For creating DataArrays based on binning of coord value
        instead of explicitly given index ranges.
    """
    return _call_cpp_func(_cpp.bins, begin, end, dim, data)


def bins_like(x: VariableLike, fill_value: _cpp.Variable) -> _cpp.Variable:
    """Create a binned variable by "broadcasting" fill values to bins of given sizes.

    The dimensions and shape of ``fill_value`` must be such that they can be broadcast
    to those of ``x``. Each element of ``fill_value`` defines the values of all the bin
    elements of the corresponding bin. The output shares the bin indices of ``x``.

    Parameters
    ----------
    x:
        Binned variable or data array serving as prototype for bin sizes.
    fill_value:
        Fill values to use for the bins.

    Returns
    -------
    :
        Variable containing fill value in bins.
    """

    var = x
    if not isinstance(x, _cpp.Variable):
        var = var.data
    return _call_cpp_func(_cpp.bins_like, var, fill_value)
