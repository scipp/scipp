# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import uuid
from typing import Iterable, List
from .typing import VariableLike
from .core import concat, reduction


class BinsReducer:
    def __init__(self, obj: VariableLike, dim: str):
        self._obj = obj
        self._dim = dim

    def concat(self):
        """Element-wise 'concat' across bins of inputs passed to :py:func:`scipp.reduce`."""  # noqa: E501
        return self._obj.bins.concat(self._dim)


class Reducer:
    def __init__(self, x: List[VariableLike]):
        self._dim = uuid.uuid4().hex
        # concat in init avoids repeated costly step in case of multiple reductions
        self._obj = concat(x, dim=self._dim)

    @property
    def bins(self):
        return BinsReducer(self._obj, self._dim)

    def all(self):
        """Element-wise 'all' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.all(self._obj, self._dim)

    def any(self):
        """Element-wise 'any' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.any(self._obj, self._dim)

    def max(self):
        """Element-wise 'max' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.max(self._obj, self._dim)

    def min(self):
        """Element-wise 'min' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.min(self._obj, self._dim)

    def sum(self):
        """Element-wise 'sum' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.sum(self._obj, self._dim)

    def mean(self):
        """Element-wise 'mean' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.mean(self._obj, self._dim)

    def nanmax(self):
        """Element-wise 'nanmax' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.nanmax(self._obj, self._dim)

    def nanmin(self):
        """Element-wise 'nanmin' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.nanmin(self._obj, self._dim)

    def nansum(self):
        """Element-wise 'nansum' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.nansum(self._obj, self._dim)

    def nanmean(self):
        """Element-wise 'nanmean' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.nanmean(self._obj, self._dim)


def reduce(x: Iterable[VariableLike]) -> Reducer:
    """Create helper object for reduction operations over list or tuple of inputs.

    Usage examples:

      >>> a = sc.linspace(dim='x', start=0, stop=1, num=4)
      >>> b = sc.linspace(dim='x', start=0.2, stop=0.8, num=4)
      >>> sc.reduce([a, b]).max()
      <scipp.Variable> (x: 4)    float64  [dimensionless]  [0.200000, 0.400000, 0.666667, 1.000000]
      >>> sc.reduce([a, b]).sum()
      <scipp.Variable> (x: 4)    float64  [dimensionless]  [0.200000, 0.733333, 1.266667, 1.800000]

    :param x: List or tuple of variables or data arrays
    :return: Reducer helper object with methods such as ``sum()`` or ``max()``
    """  # noqa: E501
    return Reducer(list(x))
