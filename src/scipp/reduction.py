# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2021 Scipp contributors (https://github.com/scipp)
# @author Simon Heybrock
import uuid
from typing import Union, List, Tuple
from .typing import VariableLike
from .core import concat, reduction


def _make_method(f):
    def method(self) -> VariableLike:
        return f(self._obj, self._dim)

    return method


class BinsReducer:
    def __init__(self, obj: VariableLike, dim: str):
        self._obj = obj
        self._dim = dim

    def concat(self):
        return self._obj.bins.concat(self._dim)


class Reducer:
    all = _make_method(reduction.all)
    any = _make_method(reduction.any)
    max = _make_method(reduction.max)
    min = _make_method(reduction.min)
    sum = _make_method(reduction.sum)
    mean = _make_method(reduction.mean)
    nanmax = _make_method(reduction.nanmax)
    nanmean = _make_method(reduction.nanmean)
    nanmin = _make_method(reduction.nanmin)
    nansum = _make_method(reduction.nansum)

    def __init__(self, x: List[VariableLike]):
        self._dim = uuid.uuid4().hex
        # concat in init avoids repeated costly step in case of multiple reductions
        self._obj = concat(x, dim=self._dim)

    @property
    def bins(self):
        return BinsReducer(self._obj, self._dim)


def reduce(x: Union[List[VariableLike], Tuple[VariableLike]]) -> Reducer:
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
