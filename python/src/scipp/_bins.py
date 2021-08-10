# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from typing import Dict, Optional, Sequence, Union

from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func
from .typing import VariableLike, MetaDataMap


class lookup:
    def __init__(self, func: _cpp.DataArray, dim: str):
        self.func = func
        self.dim = dim


class Bins:
    """
    Proxy for access to bin contents and operations on bins of a variable.

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
    def constituents(self) -> Dict[str, Union[str, _cpp.Variable, _cpp.DataArray]]:
        """Constituents of binned data, as supported by :py:func:`sc.bins`."""
        begin_end = _cpp.bins_begin_end(self._data())
        return {
            'begin': begin_end[0],
            'end': begin_end[1],
            'dim': _cpp.bins_dim(self._data()),
            'data': _cpp.bins_data(self._data())
        }

    def sum(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Sum of each bin.

        :return: The sum of each of the input bins.
        :seealso: :py:func:`scipp.sum` for summing non-bin data
        """
        return _call_cpp_func(_cpp.buckets.sum, self._obj)

    def size(self) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Number of events or elements in a bin.

        :return: The number of elements in each of the input bins.
        """
        return _call_cpp_func(_cpp.bin_size, self._obj)

    def concatenate(
            self,
            other: Optional[Union[_cpp.Variable, _cpp.DataArray]] = None,
            *,
            dim: Optional[str] = None,
            out: Optional[_cpp.DataArray] = None
    ) -> Union[_cpp.Variable, _cpp.DataArray]:
        """Concatenate bins element-wise by concatenating bin contents along
        their internal bin dimension.

        This can be used as a binary operation, or a reduction operation: The
        bins to concatenate are either obtained element-wise from `self`
        and `other`, or, if `dim` but not `other` is given, from all bins
        along the given dimension.

        :param other: Optional input containing bins.
        :param dim: Optional dimension along which to merge bins. If not given
                    and `other` is `None`, the bins are merged along all
                    dimensions.
        :param out: Optional output buffer.
        :raises: If `other` is not binned data.
        :return: The bins of the two inputs merged.
        """
        if other is not None and dim is not None:
            raise RuntimeError(
                "`concatenate` requires `other` or a `dim`, but not both.")
        if other is not None:
            if out is None:
                return _call_cpp_func(_cpp.buckets.concatenate, self._obj, other)
            else:
                if self._obj is out:
                    _call_cpp_func(_cpp.buckets.append, self._obj, other)
                else:
                    out = _call_cpp_func(_cpp.buckets.concatenate, self._obj, other)
                return out
        if out is not None:
            raise RuntimeError("`out` arg not supported for concatenate along dim")
        if dim is not None:
            return _call_cpp_func(_cpp.buckets.concatenate, self._obj, dim)
        raise RuntimeError("Reduction along all dims not supported yet.")


class GroupbyBins:
    """
    Proxy for operations on bins of a groupby object
    """
    def __init__(self, obj):
        self._obj = obj

    def concatenate(self, dim):
        return self._obj.concatenate(dim)


def _events(obj):
    """
    Returns the data underlying the bins stored in the object, or None if the
    object stores dense data.
    """
    if _cpp.is_bins(obj):
        if isinstance(obj, _cpp.Variable):
            return _cpp.bins_data(obj)
        else:
            return _cpp.bins_data(obj.data)
    else:
        return None


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


def histogram(x: Union[_cpp.DataArray, _cpp.Dataset], *,
              bins: _cpp.Variable) -> Union[_cpp.DataArray, _cpp.Dataset]:
    """Create dense data by histogramming data along all dimension given by
    edges.

    :return: DataArray / Dataset with values equal to the sum
             of values in each given bin.
    :seealso: :py:func:`scipp.bin` for binning data.
    """
    return _call_cpp_func(_cpp.histogram, x, bins)


def bin(x: _cpp.DataArray,
        *,
        edges: Optional[Sequence[_cpp.Variable]] = None,
        groups: Optional[Sequence[_cpp.Variable]] = None,
        erase: Optional[Sequence[_cpp.Variable]] = None) -> _cpp.DataArray:
    """Create binned data by binning input along all dimensions given by edges.

    This does not histogram the data, each output bin will contain a "list" of
    input values.

    At least one argument of ``edges`` and ``groups`` is required.

    :param x: Input data.
    :param edges: Bin edges, one per dimension to bin in.
    :param groups: Keys to group input by one per dimension to group in.
    :param erase: Dimension labels to remove from output.
    :return: Binned ``x``.
    :seealso: :py:func:`scipp.histogram` for histogramming data,
              :py:func:`scipp.bins` for creating binned data based on
              explicitly given index ranges.
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
    ``dtype=sc.dtype.int64``. The output dims and shape are given by ``begin``.
    If only ``begin`` is given, each bucket is a slice containing a non-range
    slice of ``data`` at the given indices. If neither ``begin`` nor ``end``
    are given, the output has ``dims=[dim]`` and contains all non-range slices
    along that dimension.

    :param begin: Optional begin indices of bins, used for slicing ``data``.
                  If not provided each row of ``data`` is mapped to a different
                  bin.
    :param end: Optional end indices of bins, used for slicing ``data``. If not
                provided this is assumed to be ``begin + 1``.
    :param dim: Dimension of ``data`` that will be sliced to obtain data for
                any given bin.
    :param data: A variable, data array, or dataset containing combined data
                 of all bins.
    :return: Variable containing data in bins.
    :seealso: :py:func:`scipp.bin` for creating DataArrays based on
              binning of coord value instead of explicitly given index ranges.
    """
    return _call_cpp_func(_cpp.bins, begin, end, dim, data)
