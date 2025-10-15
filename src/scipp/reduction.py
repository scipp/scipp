# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2023 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import uuid
from collections.abc import Iterable, Sequence
from typing import Generic, TypeVar

from .core import DataArray, Dataset, Variable, concat, reduction

_O = TypeVar("_O", Variable, DataArray, Dataset)


class BinsReducer(Generic[_O]):
    def __init__(self, obj: _O, dim: str) -> None:
        self._obj: _O = obj
        self._dim = dim

    def concat(self) -> _O:
        """Element-wise 'concat' across bins of inputs passed to :py:func:`scipp.reduce`."""  # noqa: E501
        if not self._obj.is_binned:
            raise ValueError("Input must be binned")
        return self._obj.bins.concat(self._dim)


class Reducer(Generic[_O]):
    _DIM = uuid.uuid4().hex

    def __init__(self, x: Sequence[_O]) -> None:
        # concat in init avoids repeated costly step in case of multiple reductions
        self._obj: _O = concat(x, dim=self._DIM)

    @property
    def bins(self) -> BinsReducer[_O]:
        return BinsReducer(self._obj, self._DIM)

    def all(self) -> _O:
        """Element-wise 'all' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.all(self._obj, self._DIM)

    def any(self) -> _O:
        """Element-wise 'any' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.any(self._obj, self._DIM)

    def max(self) -> _O:
        """Element-wise 'max' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.max(self._obj, self._DIM)

    def min(self) -> _O:
        """Element-wise 'min' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.min(self._obj, self._DIM)

    def sum(self) -> _O:
        """Element-wise 'sum' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.sum(self._obj, self._DIM)

    def mean(self) -> _O:
        """Element-wise 'mean' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.mean(self._obj, self._DIM)

    def nanmax(self) -> _O:
        """Element-wise 'nanmax' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.nanmax(self._obj, self._DIM)

    def nanmin(self) -> _O:
        """Element-wise 'nanmin' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.nanmin(self._obj, self._DIM)

    def nansum(self) -> _O:
        """Element-wise 'nansum' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.nansum(self._obj, self._DIM)

    def nanmean(self) -> _O:
        """Element-wise 'nanmean' across inputs passed to :py:func:`scipp.reduce`."""
        return reduction.nanmean(self._obj, self._DIM)


def reduce(x: Iterable[_O]) -> Reducer[_O]:
    """Create helper object for reduction operations over list or tuple of inputs.

    Usage examples:

      >>> a = sc.linspace(dim='x', start=0, stop=1, num=4)
      >>> b = sc.linspace(dim='x', start=0.2, stop=0.8, num=4)
      >>> sc.reduce([a, b]).max()
      <scipp.Variable> (x: 4)    float64  [dimensionless]  [0.2, 0.4, 0.666667, 1]
      >>> sc.reduce([a, b]).sum()
      <scipp.Variable> (x: 4)    float64  [dimensionless]  [0.2, 0.733333, 1.26667, 1.8]

    :param x: List or tuple of variables or data arrays
    :return: Reducer helper object with methods such as ``sum()`` or ``max()``
    """
    return Reducer(list(x))
