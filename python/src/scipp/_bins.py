# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2020 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
from ._scipp import core as _cpp
from ._cpp_wrapper_util import call_func as _call_cpp_func


class BinsGroupby:
    def __init__(self, obj, dim):
        self._obj = obj
        self._dim = dim

    def __mul__(self, histogram):
        copy = self._obj.copy()
        _cpp.buckets.scale(copy, histogram, self._dim)
        return copy

    def __truediv__(self, histogram):
        copy = self._obj.copy()
        _cpp.buckets.scale(copy, _cpp.reciprocal(histogram), self._dim)
        return copy

    def __imul__(self, histogram):
        _cpp.buckets.scale(self._obj, histogram, self._dim)

    def __itruediv__(self, histogram):
        _cpp.buckets.scale(self._obj, _cpp.reciprocal(histogram), self._dim)


class BinsGroupbyProperty:
    def __init__(self, obj):
        self._obj = obj

    def __getitem__(self, dim):
        return BinsGroupby(self._obj, dim)

    def __setitem__(self, dim, item):
        # Required for inplace arithmetic operators
        assert item is None


class Bins:
    """
    Proxy for operations on bins of a variable
    """
    def __init__(self, obj):
        self._obj = obj

    def _data(self):
        try:
            return self._obj.data
        except AttributeError:
            return self._obj

    @property
    def begin(self):
        """Begin index of bins as view of internal data buffer"""
        return _cpp.bins_begin_end(self._data())[0]

    @property
    def end(self):
        """End index of bins as view of internal data buffer"""
        return _cpp.bins_begin_end(self._data())[1]

    @property
    def dim(self):
        """Dimension of internal data buffer used for slicing into bins"""
        return _cpp.bins_dim(self._data())

    @property
    def data(self):
        """Internal data buffer holding data of all bins"""
        return _cpp.bins_data(self._data())

    @property
    def groupby(self):
        """Proxy property for operations aware of coords within bins"""
        return BinsGroupbyProperty(self._obj)

    def sum(self):
        """Sum of each bin.

        :return: The sum of each of the input bins.
        :seealso: :py:func:`scipp.sum` for summing non-bin data
        """
        return _call_cpp_func(_cpp.buckets.sum, self._obj)

    def size(self):
        """Number of events or elements in a bin.

        :return: The number of elements in each of the input bins.
        """
        return _call_cpp_func(_cpp.bin_size, self._obj)

    def join(self, other=None, dim=None, out=None):
        """Join bins element-wise by concatenating bin contents along their
        internal bin dimension.

        The bins to join are either obtained element-wise from `self` and
        `other`, or, if `dim` but not `other` is given, from all bins along
        the given dimension.

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
                "`join` requires either `other` or a `dim`, but not both.")
        if other is not None:
            if out is None:
                return _call_cpp_func(_cpp.buckets.concatenate, self._obj,
                                      other)
            else:
                if self._obj is out:
                    _call_cpp_func(_cpp.buckets.append, self._obj, other)
                else:
                    out = _call_cpp_func(_cpp.buckets.concatenate, self._obj,
                                         other)
                return out
        if out is not None:
            raise RuntimeError("`out` arg not support for join along dim")
        if dim is not None:
            return _call_cpp_func(_cpp.buckets.concatenate, self._obj, dim)
        raise RuntimeError("Reduction along all dims not supported yet.")


def _bins(obj):
    if _cpp.is_bins(obj):
        return Bins(obj)
    else:
        return None


def bin(x, edges):
    """Bin data along all dimensions given by edges.

    This does not histogram the data, each output bin will contain a "list" of
    input values.

    :return: Variable containing data in bins.
    :seealso: :py:func:`scipp.histogram` for histogramming data
    """
    return _call_cpp_func(_cpp.bucketby, x, edges)
